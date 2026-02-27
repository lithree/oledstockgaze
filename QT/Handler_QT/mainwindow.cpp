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
#include <QListWidget>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
    networkManager(new QNetworkAccessManager(this)),
    webSocket(new QWebSocket()),
    sampleTimer(new QTimer(this)),
    watchlistDelayTimer(new QTimer(this)),
    latestPrice(0.0f),
    isRunning(false),
    currentDisplayMode(DISPLAY_MODE_STOCK_PLOT),
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
    connect(watchlistDelayTimer, &QTimer::timeout, this, &MainWindow::sendWatchlistDelayed);
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
    resize(600, 700);

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

    // Display Mode Group
    QGroupBox *modeGroup = new QGroupBox("Display Mode");
    QHBoxLayout *modeLayout = new QHBoxLayout();
    
    stockPlotBtn = new QPushButton("Stock Plot");
    watchlistBtn = new QPushButton("Watchlist");
    
    modeLayout->addWidget(stockPlotBtn);
    modeLayout->addWidget(watchlistBtn);
    modeGroup->setLayout(modeLayout);
    mainLayout->addWidget(modeGroup);
    
    connect(stockPlotBtn, &QPushButton::clicked, this, &MainWindow::switchToStockPlot);
    connect(watchlistBtn, &QPushButton::clicked, this, &MainWindow::switchToWatchlist);

    // Watchlist Group
    QGroupBox *watchlistGroup = new QGroupBox("Watchlist Management");
    QVBoxLayout *watchlistLayout = new QVBoxLayout();
    
    QHBoxLayout *watchlistInputLayout = new QHBoxLayout();
    watchlistInputLayout->addWidget(new QLabel("Add Ticker:"));
    watchlistTickerInput = new QLineEdit();
    watchlistInputLayout->addWidget(watchlistTickerInput);
    
    addWatchlistBtn = new QPushButton("Add");
    removeWatchlistBtn = new QPushButton("Remove Selected");
    watchlistInputLayout->addWidget(addWatchlistBtn);
    watchlistInputLayout->addWidget(removeWatchlistBtn);
    
    watchlistLayout->addLayout(watchlistInputLayout);
    
    watchlistDisplay = new QListWidget();
    watchlistDisplay->setMaximumHeight(150);
    watchlistLayout->addWidget(watchlistDisplay);
    
    watchlistGroup->setLayout(watchlistLayout);
    mainLayout->addWidget(watchlistGroup);
    
    connect(addWatchlistBtn, &QPushButton::clicked, this, &MainWindow::addToWatchlist);
    connect(removeWatchlistBtn, &QPushButton::clicked, this, &MainWindow::removeFromWatchlist);

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

bool MainWindow::sendModeSwitch(uint8_t mode)
{
    if (!interfaceClaimed || !usbHandle) return false;

    /* Use Control Transfer (EP0) with vendor-specific request instead of bulk transfer */
    int res = libusb_control_transfer(
        usbHandle,
        USB_REQUEST_TYPE_VENDOR_OUT,    // bmRequestType: vendor-specific, host-to-device
        USB_VENDOR_REQUEST_MODE_SWITCH, // bRequest: 0x02 for mode switch
        (uint16_t)mode,                 // wValue: display mode
        0,                              // wIndex: not used
        nullptr,                        // data buffer: not used
        0,                              // wLength: 0 (no data phase)
        TIMEOUT_MS
    );

    if (res < 0) {
        logMessage(QString("Mode switch failed: %1").arg(libusb_error_name(res)));
        return false;
    }

    return true;
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
    watchlistTickerInput->setEnabled(false);
    addWatchlistBtn->setEnabled(false);
    removeWatchlistBtn->setEnabled(false);

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
    // Check if this is a watchlist opening price request
    if (reply->property("requestType").toString() == "watchlistOpeningPrice") {
        onWatchlistOpeningPriceReply(reply);
        return;
    }

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

void MainWindow::onWatchlistOpeningPriceReply(QNetworkReply *reply)
{
    QString ticker = reply->property("ticker").toString();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();

        float openPrice = jsonObj["o"].toDouble();
        if (openPrice > 0.0f) {
            watchlistOpeningPrices[ticker] = openPrice;
            logMessage(QString("Fetched opening price for %1: $%2").arg(ticker).arg(openPrice, 0, 'f', 2));
        } else {
            logMessage(QString("Could not parse opening price for %1.").arg(ticker));
        }
    } else {
        logMessage(QString("Failed to fetch opening price for %1: %2").arg(ticker, reply->errorString()));
    }

    reply->deleteLater();
}

void MainWindow::onWsConnected()
{
    logMessage(QString("WebSocket connected. Subscribing to %1...").arg(currentTicker));

    QJsonObject subscribeMsg;
    subscribeMsg["type"] = "subscribe";
    subscribeMsg["symbol"] = currentTicker;

    QJsonDocument doc(subscribeMsg);
    webSocket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    // Subscribe to all watchlist tickers
    subscribeToWatchlist();

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
            QString symbol = tradeObj["s"].toString().trimmed().toUpper();
            float price = tradeObj["p"].toDouble();

            // Update main ticker if it matches
            if (symbol == currentTicker) {
                latestPrice = price;
            }

            // Update watchlist price if ticker is in watchlist
            if (watchlist.contains(symbol)) {
                watchlistPrices[symbol] = price;
                updateWatchlistDisplay();
                
                // If in watchlist mode and USB is connected, send price update to device
                if (currentDisplayMode == DISPLAY_MODE_WATCHLIST && interfaceClaimed && usbHandle) {
                    sendUsbFrame(symbol, price);
                }
            }
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

void MainWindow::switchToStockPlot()
{
    if (!interfaceClaimed || !usbHandle) {
        logMessage("USB not connected. Please start the bridge first.");
        return;
    }

    // Stop the watchlist delay timer when switching to stock plot
    watchlistDelayTimer->stop();

    bool success = sendModeSwitch(DISPLAY_MODE_STOCK_PLOT);
    if (success) {
        currentDisplayMode = DISPLAY_MODE_STOCK_PLOT;
        // Resume sampling for main ticker in stock plot mode
        if (isRunning) {
            int intervalMs = static_cast<int>(intervalInput->value() * 1000);
            sampleTimer->start(intervalMs);
        }
        logMessage("Switched to Stock Plot mode");
    } else {
        logMessage("Failed to switch to Stock Plot mode");
    }
}

void MainWindow::switchToWatchlist()
{
    if (!interfaceClaimed || !usbHandle) {
        logMessage("USB not connected. Please start the bridge first.");
        return;
    }

    // Stop the sampling timer when switching to watchlist mode
    sampleTimer->stop();

    bool success = sendModeSwitch(DISPLAY_MODE_WATCHLIST);
    if (success) {
        currentDisplayMode = DISPLAY_MODE_WATCHLIST;
        logMessage("Switched to Watchlist mode");
        // Send watchlist items after a small delay to allow device to finish mode switch
        // Stop any existing timer first to ensure clean state
        watchlistDelayTimer->stop();
        watchlistDelayTimer->setSingleShot(true);
        watchlistDelayTimer->start(300);  // 300ms delay for reliable mode switching
    } else {
        logMessage("Failed to switch to Watchlist mode");
        // Resume sampling if mode switch failed
        if (isRunning) {
            int intervalMs = static_cast<int>(intervalInput->value() * 1000);
            sampleTimer->start(intervalMs);
        }
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
    watchlistTickerInput->setEnabled(true);
    addWatchlistBtn->setEnabled(true);
    removeWatchlistBtn->setEnabled(true);
}

void MainWindow::addToWatchlist()
{
    QString ticker = watchlistTickerInput->text().trimmed().toUpper();
    if (ticker.isEmpty()) {
        logMessage("Please enter a ticker symbol.");
        return;
    }

    if (watchlist.contains(ticker)) {
        logMessage(QString("%1 is already in the watchlist.").arg(ticker));
        return;
    }

    watchlist.append(ticker);
    watchlistPrices[ticker] = 0.0f;
    watchlistOpeningPrices[ticker] = 0.0f;
    watchlistTickerInput->clear();
    updateWatchlistDisplay();
    logMessage(QString("Added %1 to watchlist. Fetching opening price...").arg(ticker));

    // Fetch opening price from REST API
    QString urlStr = QString("https://finnhub.io/api/v1/quote?symbol=%1&token=%2").arg(ticker, apiKey);
    QUrl url(urlStr);
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    
    // Tag the reply to identify it as a watchlist opening price request
    reply->setProperty("requestType", "watchlistOpeningPrice");
    reply->setProperty("ticker", ticker);

    // If WebSocket is connected, subscribe to the new ticker
    if (webSocket->isValid()) {
        QJsonObject subscribeMsg;
        subscribeMsg["type"] = "subscribe";
        subscribeMsg["symbol"] = ticker;

        QJsonDocument doc(subscribeMsg);
        webSocket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    }
}

void MainWindow::removeFromWatchlist()
{
    QListWidgetItem *item = watchlistDisplay->currentItem();
    if (!item) {
        logMessage("Please select a ticker to remove.");
        return;
    }

    QString ticker = item->text().split(" - ")[0].trimmed();
    watchlist.removeAll(ticker);
    watchlistPrices.remove(ticker);
    updateWatchlistDisplay();
    logMessage(QString("Removed %1 from watchlist.").arg(ticker));
}

void MainWindow::updateWatchlistDisplay()
{
    watchlistDisplay->clear();
    for (const QString &ticker : watchlist) {
        float price = watchlistPrices[ticker];
        QString displayText;
        if (price > 0.0f) {
            displayText = QString("%1 - $%2").arg(ticker).arg(price, 0, 'f', 2);
        } else {
            displayText = QString("%1 - N/A").arg(ticker);
        }
        watchlistDisplay->addItem(displayText);
    }
}

void MainWindow::subscribeToWatchlist()
{
    for (const QString &ticker : watchlist) {
        QJsonObject subscribeMsg;
        subscribeMsg["type"] = "subscribe";
        subscribeMsg["symbol"] = ticker;

        QJsonDocument doc(subscribeMsg);
        webSocket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    }
}

void MainWindow::sendWatchlistToDevice()
{
    if (!interfaceClaimed || !usbHandle) return;

    // First, send opening prices to establish the baseline on the device
    for (const QString &ticker : watchlist) {
        float openingPrice = watchlistOpeningPrices[ticker];
        if (openingPrice > 0.0f) {
            bool success = sendUsbFrame(ticker, openingPrice);
            logMessage(QString("Sent opening price for %1: $%2 (Success: %3)")
                           .arg(ticker)
                           .arg(openingPrice, 0, 'f', 2)
                           .arg(success ? "Yes" : "No"));
        }
    }
    
    // Then send current prices, which will show correct percentage changes
    for (const QString &ticker : watchlist) {
        float price = watchlistPrices[ticker];
        if (price > 0.0f) {
            bool success = sendUsbFrame(ticker, price);
            logMessage(QString("Sent current price for %1: $%2 (Success: %3)")
                           .arg(ticker)
                           .arg(price, 0, 'f', 2)
                           .arg(success ? "Yes" : "No"));
        }
    }
}

void MainWindow::sendWatchlistDelayed()
{
    // Stop the delay timer and send watchlist items
    watchlistDelayTimer->stop();
    sendWatchlistToDevice();
}
