#include "mainwindow.h"
#include "ui_mainwindow.h"

// Qt networking + UI + utility headers
#include <QHostAddress>        //it helps us identify the computer or server we want to communicate with over a network.
#include <QDebug>            // display the debug information on output window.
#include <QHeaderView>         // controls the headers of widgets

#include <QTableWidgetItem>   // used to work with individual items/cells in QTableWidget
#include <QDateTime>          // it is used to work with date and time
#include <QFont>
#include <QAbstractSocket>    // // It helps us manage and check the status of the network connection.

#include <QMessageBox>  // It helps us display the information on the message box.

// Constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , server(new QTcpServer(this))   // It helps us create the Server.
    , clientSocket(nullptr)          //initially there is no client is connected.
{
    ui->setupUi(this);               // Setup UI from .ui file

    setupTable();                    // Initialize table UI

    //It hepls us start the TCP server on port 12345
    if (!server->listen(QHostAddress::Any, 12345)) {
        //If the server cannot start, show error message.
        qDebug() << "❌ Server failed to start:" << server->errorString();
        return;
    }

    // When a new client connects → call newConnection()
    connect(server, &QTcpServer::newConnection, this, &MainWindow::newConnection);
      // Display a message showing that the server is running
    qDebug() << "✅ Server listening on port" << server->serverPort(); // Display a message showing that the server is running
}

// Destructor (cleanup)
MainWindow::~MainWindow()
{
   //check Whether a client socket exists
    if (clientSocket) {
        clientSocket->close(); //close the connection with the client
        clientSocket->deleteLater();// It delete the socket safely
        clientSocket = nullptr;
    }

    //Check whether The TCP server exists.
    if (server) {
        server->close();   // Stop the server from listening
        server->deleteLater(); // Stop the server from listening
        server = nullptr;   // Set server pointer to nullptr
    }

    delete ui; // Delete UI
}

// Functions used to configure the table.
void MainWindow::setupTable()
{
    ui->tableWidget->setColumnCount(5); // set the table to contain 5 columns.

   // Create names for the five columns.
    QStringList headers = {"Product Name", "Volume", "Market Capital", "Credit Rating", "Created_at"};
    ui->tableWidget->setHorizontalHeaderLabels(headers);  //Display the column names in the table header.


    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows); // It helps us user selects a cell the complete row will be selected
    ui->tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);  // It allow the user to select multiple rows.

    // Stretch columns to fit width
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Table styling
    ui->tableWidget->setShowGrid(true);
    ui->tableWidget->setGridStyle(Qt::SolidLine);
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->setStyleSheet("alternate-background-color: #f2f2f2; background-color: white;");

    // Row height
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(25);
    ui->tableWidget->verticalHeader()->setVisible(false);

    // Enable sorting
    ui->tableWidget->setSortingEnabled(true);

    // Make header bold
    QFont headerFont = ui->tableWidget->horizontalHeader()->font();
    headerFont.setBold(true);
    ui->tableWidget->horizontalHeader()->setFont(headerFont);

    // Header styling
    ui->tableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { background-color: lightgray; padding: 4px; border: 1px solid gray; }"
    );

    // Center header text
    ui->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
}

// Handle new client connection
void MainWindow::newConnection()
{
    // check if another client is already connected.
    if (clientSocket) {
        clientSocket->close();// close the old client connection.
        clientSocket->deleteLater();
        clientSocket = nullptr;
    }


    clientSocket = server->nextPendingConnection();// Accept the new client connection and  nextPendingConnection() gives us the client's socket
    if (!clientSocket) {
        qDebug() << "❌ Failed to accept incoming connection";  // print an error if the connection failed.
        return;
    }


    connect(clientSocket, &QTcpSocket::connected, this, &MainWindow::socketConnected); // the socket becomes connected call socketconnected().
    connect(clientSocket, &QTcpSocket::disconnected, this, &MainWindow::socketDisconnected); // the client disconnects call socketDisconnected().
    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::socketReadyRead); //the client readyread call socketReadyread().


    connect(clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &MainWindow::handleSocketError);  // the socket error occurs call handleSocketError().

    qDebug() << "🔌 New connection from"
             << clientSocket->peerAddress().toString()
             << ":" << clientSocket->peerPort();
}

// Function called when the client connection is established
void MainWindow::socketConnected()
{
    qDebug() << "✅ Client connected";
}

// Function called when the client disconnects
void MainWindow::socketDisconnected()
{
    qDebug() << "❌ Client disconnected";

    if (clientSocket) {
        clientSocket->deleteLater();
        clientSocket = nullptr;
    }
}

// Function used to receive data from the client
void MainWindow::socketReadyRead()
{
    if (!clientSocket) return;

    // Read all available data from the client.
    QByteArray data = clientSocket->readAll();
    QString receivedData = QString::fromUtf8(data);

    // Qt version check for string splitting enum
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList lines = receivedData.split("\n", Qt::SkipEmptyParts);  // Split received data into separate lines
#else
    QStringList lines = receivedData.split("\n", QString::SkipEmptyParts);
#endif


    static bool collectingRows = false; // This variable tells us whether we are currently receiving multiple rows
    static QStringList collectedRows;

    // Process each line
    for (const QString &rawLine : lines)
    {
         // Remove unnecessary spaces from the beginning and end
        QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;


        // --------------------------------------------------
        // STEP 1: Client asks permission to send rows
        // --------------------------------------------------

        // Check whether the received message is REQUEST_TRANSFER
        if (!collectingRows && line.compare("REQUEST_TRANSFER", Qt::CaseInsensitive) == 0) {
            clientSocket->write("READY_FOR_ROWS\n"); // Tell the client that the server is ready
            clientSocket->flush(); // send the data immediately
            continue;
        }

         // --------------------------------------------------
        // STEP 2: Start receiving rows
        // --------------------------------------------------

        // Check whether the client sent ROWS_START
        if (line.compare("ROWS_START", Qt::CaseInsensitive) == 0) {
            collectingRows = true; //start collecting rows
            collectedRows.clear();
            continue;
        }

        // --------------------------------------------------
       // STEP 3: Stop receiving rows
       // --------------------------------------------------

         // Check whether the client sent ROWS_END
        if (line.compare("ROWS_END", Qt::CaseInsensitive) == 0) {
            collectingRows = false;

            // No rows received
            if (collectedRows.isEmpty()) {
                QMessageBox::warning(this, "Empty Table", "No rows received.");
                clientSocket->write("TRANSFER_RESULT:EMPTY\n");
                clientSocket->flush();
            } else {

                // Process each CSV row
                for (const QString &csvRow : collectedRows) {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                    QStringList fields = csvRow.split(",", Qt::KeepEmptyParts);
#else
                    QStringList fields = csvRow.split(",", QString::KeepEmptyParts);
#endif

                    // Extract Product Name and Credit Rating
                    QString productName = fields.value(0).trimmed();
                    QString volume      = ""; // Forced empty (2nd Field)
                    QString marketCap   = ""; // Forced empty (3rd Field)
                    QString credit      = fields.value(3).trimmed();

                    // Add timestamp
                    QString createdAt = QDateTime::currentDateTime()
                                        .toString("yyyy-MM-dd HH:mm:ss");

                    // Build row
                    QStringList rowFields;
                    rowFields << productName << volume << marketCap << credit << createdAt;

                    insertRowToTable(rowFields); // Insert into table
                }

                // Send success response to client
                clientSocket->write(QString("TRANSFER_RESULT:OK:%1\n")
                                        .arg(collectedRows.size()).toUtf8());
                clientSocket->flush();
            }

            collectedRows.clear();
            continue;
        }

        // If inside ROWS block → store rows
        if (collectingRows) {
            collectedRows << line;
            continue;
        }

        // Handle single row outside block
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        QStringList fields = line.split(",", Qt::KeepEmptyParts);
#else
        QStringList fields = line.split(",", QString::KeepEmptyParts);
#endif

        if (fields.size() >= 1) {
            QStringList rowFields;
            rowFields << fields.value(0).trimmed()
                      << "" // Forced empty (2nd Field)
                      << "" // Forced empty (3rd Field)
                      << fields.value(3).trimmed()
                      << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

            insertRowToTable(rowFields);
        }
    }
}

// Insert row into table
void MainWindow::insertRowToTable(const QStringList &fields)
{
    const int expectedCols = 5;

    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    for (int col = 0; col < expectedCols; ++col) {

        QString text = (col < fields.size()) ? fields.at(col) : QString();

        QTableWidgetItem *item = new QTableWidgetItem(text);

        // Make cell read-only
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);

        // Center align text
        item->setTextAlignment(Qt::AlignCenter);

        ui->tableWidget->setItem(row, col, item);
    }
}

// Handle socket errors
void MainWindow::handleSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);

    if (clientSocket)
        qDebug() << "⚠️ Socket error:" << clientSocket->errorString();
}

// When a cell is clicked
void MainWindow::on_tableWidget_cellActivated(int row, int column)
{
    QTableWidgetItem *it = ui->tableWidget->item(row, column);

    QString value = it ? it->text() : QString();

    qDebug() << "📌 Cell (" << row << "," << column << "):" << value;
}
