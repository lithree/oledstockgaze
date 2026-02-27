#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QWebSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QListWidget>
#include <QMap>
#include "libusb.h"

/* USB Protocol Constants */
#define VID 0x1A86
#define PID 0x5537
#define EP_OUT 0x03
#define TIMEOUT_MS 1000
#define FRAME_SOF 0x0A
#define MSG_TYPE_TICKER_AND_PRICE 0x03
#define MSG_TYPE_MODE_SWITCH 0x04
#define DISPLAY_MODE_STOCK_PLOT 1
#define DISPLAY_MODE_WATCHLIST 2
#define TICKER_FIXED_LEN 8
#define COMBINED_PAYLOAD_LEN (TICKER_FIXED_LEN + sizeof(float))
#define OVERHEAD 6 // SOF(2) + Type(1) + Len(2) + Checksum(1)
#define MAX_FRAME 1030

/* USB Control Transfer Constants for Mode Switch */
#define USB_VENDOR_REQUEST_MODE_SWITCH 0x02
#define USB_REQUEST_TYPE_VENDOR_OUT 0x40

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startBridge();
    void stopBridge();
    void sendSample();
    void onRestReply(QNetworkReply *reply);
    void onWsConnected();
    void onWsMessageReceived(const QString &message);
    void onWsError(QAbstractSocket::SocketError error);
    void onWsClosed();
    void switchToStockPlot();
    void switchToWatchlist();
    void addToWatchlist();
    void removeFromWatchlist();
    void onWatchlistOpeningPriceReply(QNetworkReply *reply);

private:
    void setupUi();
    void logMessage(const QString &msg);
    bool initUsb();
    void closeUsb();
    bool sendUsbFrame(const QString &ticker, float price);
    bool sendModeSwitch(uint8_t mode);
    void subscribeToWatchlist();
    void updateWatchlistDisplay();
    void sendWatchlistToDevice();
    void sendWatchlistDelayed();

    // UI Elements
    QLineEdit *tickerInput;
    QDoubleSpinBox *intervalInput;
    QPushButton *startBtn;
    QPushButton *stopBtn;
    QPushButton *stockPlotBtn;
    QPushButton *watchlistBtn;
    QLineEdit *watchlistTickerInput;
    QPushButton *addWatchlistBtn;
    QPushButton *removeWatchlistBtn;
    QListWidget *watchlistDisplay;
    QTextEdit *consoleLog;

    // Network & Timing
    QNetworkAccessManager *networkManager;
    QWebSocket *webSocket;
    QTimer *sampleTimer;
    QTimer *watchlistDelayTimer;

    // State
    QString apiKey;
    QString currentTicker;
    float latestPrice;
    bool isRunning;
    uint8_t currentDisplayMode;  // Track current display mode
    QStringList watchlist;
    QMap<QString, float> watchlistPrices;
    QMap<QString, float> watchlistOpeningPrices;  // Store actual opening prices from API

    // USB State
    libusb_context *usbCtx;
    libusb_device_handle *usbHandle;
    bool interfaceClaimed;
};

#endif // MAINWINDOW_H
