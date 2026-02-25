#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QProcessEnvironment>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
    networkManager(new QNetworkAccessManager(this)),
    webSocket(new QWebSocket()),
    sampleTimer(new QTimer(this)),
    latestPrice(0.0f),
    isRunning(false),
    usbCtx(nullptr),
    usbHandle(nullptr),
    interfaceClaimed(false)
{
    setupUi();

    // Retrieve API key from environment variables
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    apiKey = env.value("FINNHUB_API_KEY");

    if (apiKey.isEmpty()) {
        logMessage("ERROR: FINNHUB_API_KEY environment variable is not set.");
        startBtn->setEnabled(false);
    }

    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onRestReply);
    connect(webSocket, &QWebSocket::connected, this, &MainWindow::onWsConnected);
    connect(webSocket, &QWebSocket::textMessageReceived, this, &MainWindow::onWsMessageReceived);
    connect(webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred), this, &MainWindow::onWsError);
    connect(webSocket, &QWebSocket::disconnected, this, &MainWindow::onWsClosed);
    connect(sampleTimer, &QTimer::timeout, this, &MainWindow::sendSample);
}

MainWindow::~MainWindow()
{
    stopBridge();
}

void MainWindow::setupUi()
{
    setWindowTitle("Finnhub Bridge");
    resize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Settings Group
    QGroupBox *settingsGroup = new QGroupBox("Settings");
    QHBoxLayout *settingsLayout = new QHBoxLayout();

    settingsLayout->addWidget(new QLabel("Ticker:"));
    tickerInput = new QLineEdit("AMD");
    settingsLayout->addWidget(tickerInput);

    settingsLayout->addWidget(new QLabel("Interval (s):"));
    intervalInput = new QDoubleSpinBox();
    intervalInput->setValue(1.0);
    intervalInput->setSingleStep(0.1);
    intervalInput->setMinimum(0.1);
    settingsLayout->addWidget(intervalInput);

    settingsGroup->setLayout(settingsLayout);
    mainLayout->addWidget(settingsGroup);

    // Control Group
    QHBoxLayout *controlLayout = new QHBoxLayout();
    startBtn = new QPushButton("Start Data Stream");
    stopBtn = new QPushButton("Stop Stream");
    stopBtn->setEnabled(false);

    controlLayout->addWidget(startBtn);
    controlLayout->addWidget(stopBtn);
    mainLayout->addLayout(controlLayout);

    connect(startBtn, &QPushButton::clicked, this, &MainWindow::startBridge);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::stopBridge);

    // Log Area
    QGroupBox *logGroup = new QGroupBox("Console Output");
    QVBoxLayout *logLayout = new QVBoxLayout();
    consoleLog = new QTextEdit();
    consoleLog->setReadOnly(true);
    logLayout->addWidget(consoleLog);
    logGroup->setLayout(logLayout);
    mainLayout->addWidget(logGroup);
}

void MainWindow::logMessage(const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    consoleLog->append(QString("[%1] %2").arg(timestamp, msg));
}

bool MainWindow::initUsb()
{
    int result = libusb_init(&usbCtx);
    if (result < 0) {
        logMessage("libusb_init failed");
        return false;
    }

    usbHandle = libusb_open_device_with_vid_pid(usbCtx, VID, PID);
    if (!usbHandle) {
        logMessage("USB Device not found. Check connection.");
        libusb_exit(usbCtx);
        usbCtx = nullptr;
        return false;
    }

    libusb_set_auto_detach_kernel_driver(usbHandle, 1);

    result = libusb_claim_interface(usbHandle, 0);
    if (result < 0) {
        logMessage(QString("Failed to claim interface: %1").arg(libusb_error_name(result)));
        libusb_close(usbHandle);
        libusb_exit(usbCtx);
        usbCtx = nullptr;
        usbHandle = nullptr;
        return false;
    }

    interfaceClaimed = true;
    logMessage("USB Interface claimed successfully.");
    return true;
}

void MainWindow::closeUsb()
{
    if (interfaceClaimed && usbHandle) {
        libusb_release_interface(usbHandle, 0);
        interfaceClaimed = false;
    }
    if (usbHandle) {
        libusb_close(usbHandle);
        usbHandle = nullptr;
    }
    if (usbCtx) {
        libusb_exit(usbCtx);
        usbCtx = nullptr;
    }
    logMessage("USB resources released.");
}

bool MainWindow::sendUsbFrame(const QString &ticker, float price)
{
    if (!interfaceClaimed || !usbHandle) return false;

    uint8_t combined_payload[COMBINED_PAYLOAD_LEN] = {0};

    QByteArray tickerBytes = ticker.toUtf8();
    int tickerLen = qMin(tickerBytes.length(), (int)TICKER_FIXED_LEN);
    memcpy(combined_payload, tickerBytes.constData(), tickerLen);
    memcpy(combined_payload + TICKER_FIXED_LEN, &price, sizeof(float));

    uint8_t frame[MAX_FRAME] = {0};
    uint8_t *ptr = frame;
    uint8_t checksum = 0;

    *ptr++ = FRAME_SOF;
    *ptr++ = FRAME_SOF;
    *ptr++ = MSG_TYPE_TICKER_AND_PRICE;
    *ptr++ = (uint8_t)(COMBINED_PAYLOAD_LEN & 0xFF);
    *ptr++ = (uint8_t)((COMBINED_PAYLOAD_LEN >> 8) & 0xFF);

    memcpy(ptr, combined_payload, COMBINED_PAYLOAD_LEN);
    ptr += COMBINED_PAYLOAD_LEN;

    for (int i = 0; i < (ptr - frame); i++) {
        checksum ^= frame[i];
    }
    *ptr = checksum;

    int transferred = 0;
    int len = COMBINED_PAYLOAD_LEN + OVERHEAD;

    int res = libusb_bulk_transfer(usbHandle, EP_OUT, frame, len, &transferred, TIMEOUT_MS);

    return (res == 0);
}

void MainWindow::startBridge()
{
    currentTicker = tickerInput->text().trimmed().toUpper();
    if (currentTicker.isEmpty()) {
        logMessage("Please enter a valid ticker.");
        return;
    }

    consoleLog->clear();
    startBtn->setEnabled(false);
    tickerInput->setEnabled(false);
    intervalInput->setEnabled(false);
    stopBtn->setEnabled(true);

    isRunning = true;
    latestPrice = 0.0f;

    if (!initUsb()) {
        stopBridge();
        return;
    }

    logMessage("Fetching opening price...");
    QString urlStr = QString("https://finnhub.io/api/v1/quote?symbol=%1&token=%2").arg(currentTicker, apiKey);
    networkManager->get(QNetworkRequest(QUrl(urlStr)));
}

void MainWindow::onRestReply(QNetworkReply *reply)
{
    if (!isRunning) {
        reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();

        float openPrice = jsonObj["o"].toDouble();
        if (openPrice > 0.0f) {
            logMessage(QString("Fetched opening price: $%1").arg(openPrice));
            sendUsbFrame(currentTicker, openPrice);
        } else {
            logMessage("Could not parse opening price.");
        }
    } else {
        logMessage(QString("REST API Error: %1").arg(reply->errorString()));
    }

    reply->deleteLater();

    // Start WebSocket after REST call completes
    QString wsUrl = QString("wss://ws.finnhub.io?token=%1").arg(apiKey);
    webSocket->open(QUrl(wsUrl));
}

void MainWindow::onWsConnected()
{
    logMessage(QString("WebSocket connected. Subscribing to %1...").arg(currentTicker));

    QJsonObject subscribeMsg;
    subscribeMsg["type"] = "subscribe";
    subscribeMsg["symbol"] = currentTicker;

    QJsonDocument doc(subscribeMsg);
    webSocket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    // Start the sampling timer based on UI input
    int intervalMs = static_cast<int>(intervalInput->value() * 1000);
    sampleTimer->start(intervalMs);
}

void MainWindow::onWsMessageReceived(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject obj = doc.object();

    if (obj["type"].toString() == "trade") {
        QJsonArray dataArray = obj["data"].toArray();
        if (!dataArray.isEmpty()) {
            QJsonObject tradeObj = dataArray[0].toObject();
            latestPrice = tradeObj["p"].toDouble();
        }
    }
}

void MainWindow::onWsError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    logMessage(QString("WebSocket Error: %1").arg(webSocket->errorString()));
}

void MainWindow::onWsClosed()
{
    logMessage("WebSocket connection closed.");
    if (isRunning) {
        logMessage("Attempting to reconnect in 5 seconds...");
        QTimer::singleShot(5000, this, [this]() {
            if (isRunning) {
                QString wsUrl = QString("wss://ws.finnhub.io?token=%1").arg(apiKey);
                webSocket->open(QUrl(wsUrl));
            }
        });
    }
}

void MainWindow::sendSample()
{
    if (latestPrice > 0.0f && isRunning) {
        bool success = sendUsbFrame(currentTicker, latestPrice);
        logMessage(QString("Sampling -> %1: %2 (Success: %3)")
                       .arg(currentTicker)
                       .arg(latestPrice, 0, 'f', 2)
                       .arg(success ? "Yes" : "No"));
    }
}

void MainWindow::stopBridge()
{
    if (!isRunning) return;

    logMessage("Initiating shutdown...");
    isRunning = false;
    sampleTimer->stop();
    webSocket->close();
    closeUsb();

    startBtn->setEnabled(true);
    tickerInput->setEnabled(true);
    intervalInput->setEnabled(true);
    stopBtn->setEnabled(false);
}
