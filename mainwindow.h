#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QVector>

#include "vm.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void run();
    void runHello();
    void runLoop();
    void runFactorial();
    void onIpChange(int newIP);
    void onSpChange(int newSP);
    void onCallSpChange(int newSP);
    void onOpcodeChange(int newOpcode);
    void highlightCurrentLine(int currentIP);
    void onVmFinished();
    void onRunAction();
    void onPauseAction();
    void onHaltAction();
    void onVmPaused(bool paused);

private:
    void updateWindowTitle();
    void updateProgramListing(int *code, int codeSize, const QString &programName);
    void runProgram(int *code, int codeSize, const QString &programName, int nglobals = 0, int ip = 0);
    QString formatBinaryDisplay(int value);
    Ui::MainWindow *ui;

    VM* vm;
    
    // Program listing data for highlighting
    int *currentCode;
    int currentCodeSize;
    QStringList programLines;
    QVector<int> lineAddresses;
    
    // Current program information
    QString currentProgramName;
    bool isRunning;
    bool isPaused;
    
    // Last program data for restart
    int *lastCode;
    int lastCodeSize;
    QString lastProgramName;
    int lastNglobals;
    int lastIp;
};

#endif // MAINWINDOW_H
