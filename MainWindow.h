#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QHash>

namespace Ui {
class MainWindow;
}

//*****************************************************************************
//*****************************************************************************
/**
 * @brief The MainWindow class
 */
//*****************************************************************************
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void handleConnect();
    void handleDisconnect();
    void handleAddName();

    void handleCalibrate();
    void handleShutdown();
    void handleCancelCalibrate();

    void handleNameSelected( QListWidgetItem* item );

    void handleWeigh();
    void handleClearLast();
    void handleDone();


private:
    Ui::MainWindow *ui;

    QStringList names_;
    QHash<QString,int> numItems_;

    bool connected_;

};

#endif // MAINWINDOW_H
