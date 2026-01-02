#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "vm.h"

int hello[] = {
    VM::ICONST, 1234,
    VM::PRINT,
    VM::ICONST, 5678,
    VM::PRINT,
    VM::HALT
};

int loop[] = {
    // .GLOBALS 2; N, I
    // N = 10                      ADDRESS
    VM::ICONST, 10,            // 0
    VM::GSTORE, 0,             // 2
    // I = 0
    VM::ICONST, 0,             // 4
    VM::GSTORE, 1,             // 6
    // WHILE I<N:
    // START (8):
    VM::GLOAD, 1,              // 8
    VM::GLOAD, 0,              // 10
    VM::ILT,                   // 12
    VM::BRF, 27,               // 13
    //     PRINT current I value
    VM::GLOAD, 1,              // 15
    VM::PRINT,                 // 17
    //     I = I + 1
    VM::GLOAD, 1,              // 18
    VM::ICONST, 1,             // 20
    VM::IADD,                  // 22
    VM::GSTORE, 1,             // 23
    VM::BR, 8,                 // 25
    // DONE (27):
    // PRINT "LOOPED "+N+" TIMES."
    VM::HALT                   // 27
};

const int FACTORIAL_ADDRESS = 0;
int factorial[] = {
    //.def factorial: ARGS=1, LOCALS=0	ADDRESS
    //	IF N < 2 RETURN 1
    VM::LOAD, 0,                // 0
    VM::ICONST, 2,              // 2
    VM::ILT,                    // 4
    VM::BRF, 10,                // 5
    VM::ICONST, 1,              // 7
    VM::RET,                    // 9
    //CONT:
    //	RETURN N * FACT(N-1)
    VM::LOAD, 0,                // 10
    VM::LOAD, 0,                // 12
    VM::ICONST, 1,              // 14
    VM::ISUB,                   // 16
    VM::CALL, FACTORIAL_ADDRESS, 1, 0,    // 17
    VM::IMUL,                   // 21
    VM::RET,                    // 22
    //.DEF MAIN: ARGS=0, LOCALS=0
    // PRINT FACT(1)
    VM::ICONST, 15,              // 23    <-- MAIN METHOD!
    VM::CALL, FACTORIAL_ADDRESS, 1, 0,    // 25
    VM::PRINT,                  // 29
    VM::HALT                    // 30
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Ensure first tab is shown on startup
    ui->tabWidget->setCurrentIndex(0);

    // Initialize program listing data
    currentCode = nullptr;
    currentCodeSize = 0;
    programLines.clear();
    lineAddresses.clear();
    
    // Initialize program name and window title
    currentProgramName = "No Program";
    isRunning = false;
    isPaused = false;
    
    // Initialize last program data
    lastCode = nullptr;
    lastCodeSize = 0;
    lastProgramName = "";
    lastNglobals = 0;
    lastIp = 0;
    
    updateWindowTitle();

    connect(ui->actionHello, &QAction::triggered, this, &MainWindow::runHello);
    connect(ui->actionLoop, &QAction::triggered, this, &MainWindow::runLoop);
    connect(ui->actionFactorial, &QAction::triggered, this, &MainWindow::runFactorial);
    
    // Connect toolbar actions
    connect(ui->actionE_xit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionRun, &QAction::triggered, this, &MainWindow::onRunAction);
    connect(ui->actionPause, &QAction::triggered, this, &MainWindow::onPauseAction);
    connect(ui->actionHalt, &QAction::triggered, this, &MainWindow::onHaltAction);

    QMetaObject::invokeMethod(this, "runHello", Qt::QueuedConnection);
}

void MainWindow::updateProgramListing(int *code, int codeSize, const QString &programName)
{
    // Store current program information for highlighting
    (void)programName; // unused
    currentCode = code;
    currentCodeSize = codeSize;
    programLines.clear();
    lineAddresses.clear();
    
    QString listing = QString();
    
    for (int i = 0; i < static_cast<int>(codeSize / sizeof(int)); i++) {
        int opcode = code[i];
        
        // Find instruction name
        QString instName = "UNKNOWN";
        switch(opcode) {
            case VM::NOOP: instName = "noop"; break;
            case VM::IADD: instName = "iadd"; break;
            case VM::ISUB: instName = "isub"; break;
            case VM::IMUL: instName = "imul"; break;
            case VM::ILT: instName = "ilt"; break;
            case VM::IEQ: instName = "ieq"; break;
            case VM::BR: instName = "br"; break;
            case VM::BRT: instName = "brt"; break;
            case VM::BRF: instName = "brf"; break;
            case VM::ICONST: instName = "iconst"; break;
            case VM::LOAD: instName = "load"; break;
            case VM::GLOAD: instName = "gload"; break;
            case VM::STORE: instName = "store"; break;
            case VM::GSTORE: instName = "gstore"; break;
            case VM::PRINT: instName = "print"; break;
            case VM::POP: instName = "pop"; break;
            case VM::CALL: instName = "call"; break;
            case VM::RET: instName = "ret"; break;
            case VM::HALT: instName = "halt"; break;
        }
        
        QString line = QString("%1: %2").arg(i, 4, 10, QLatin1Char('0')).arg(instName, -8);
        
        // Add operands based on instruction type
        int numOperands = 0;
        switch(opcode) {
            case VM::BR:
            case VM::BRT:
            case VM::BRF:
            case VM::ICONST:
            case VM::LOAD:
            case VM::GLOAD:
            case VM::STORE:
            case VM::GSTORE:
                numOperands = 1;
                break;
            case VM::CALL:
                numOperands = 3;
                break;
        }
        
        for (int j = 0; j < numOperands && (i + 1 + j) < static_cast<int>(codeSize / sizeof(int)); j++) {
            line += QString(" %1").arg(code[i + 1 + j]);
        }
        
        listing += line + "\n";
        
        // Store line information for highlighting
        programLines.append(line);
        lineAddresses.append(i);
        
        // Skip over operands
        i += numOperands;
    }
    
    ui->programListing->setPlainText(listing);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::run()
{
    ui->stdoutEdit->clear();
    ui->stack->clear();
    ui->memory->clear();
    ui->instructions->clear();

    vm = new VM(factorial, sizeof(factorial), 0);

    connect(vm, &VM::finished, vm, &QObject::deleteLater);
    vm->start();

    vm->print_data(vm->globals, vm->nglobals);
}

void MainWindow::runProgram(int *code, int codeSize, const QString &programName, int nglobals, int ip)
{
    ui->stdoutEdit->clear();
    ui->stack->clear();
    ui->memory->clear();
    ui->instructions->clear();

    vm = new VM(code, codeSize, nglobals, ip);

    connect(vm, SIGNAL(hasStdout(QString)), ui->stdoutEdit, SLOT(appendPlainText(QString)));
    connect(vm, SIGNAL(hasStack(QString)),  ui->stack, SLOT(appendPlainText(QString)));
    connect(vm, SIGNAL(hasMemory(QString)), ui->memory, SLOT(appendPlainText(QString)));
    connect(vm, SIGNAL(hasInstruction(QString)), ui->instructions, SLOT(appendPlainText(QString)));
    connect(vm, SIGNAL(ipChanged(int)), this, SLOT(onIpChange(int)));
    connect(vm, SIGNAL(ipChanged(int)), this, SLOT(highlightCurrentLine(int)));
    connect(vm, SIGNAL(spChanged(int)), this, SLOT(onSpChange(int)));
    connect(vm, SIGNAL(callSpChanged(int)), this, SLOT(onCallSpChange(int)));
    connect(vm, SIGNAL(opcodeChanged(int)), this, SLOT(onOpcodeChange(int)));
    connect(vm, SIGNAL(finished()), this, SLOT(onVmFinished()));
    connect(vm, SIGNAL(finished()), vm, SLOT(deleteLater()));
    connect(vm, SIGNAL(pausedChanged(bool)), this, SLOT(onVmPaused(bool)));
    
    // Update window title with current program name
    currentProgramName = programName;
    isRunning = true;
    
    // Store last program data for restart
    lastCode = code;
    lastCodeSize = codeSize;
    lastProgramName = programName;
    lastNglobals = nglobals;
    lastIp = ip;
    
    updateWindowTitle();
    
    updateProgramListing(code, codeSize, programName);
    vm->start();
}

void MainWindow::runHello()
{
    runProgram(hello, sizeof(hello), "Hello Program", 0, 0);
}

void MainWindow::runLoop()
{
    runProgram(loop, sizeof(loop), "Loop Program", 2, 0);
}

void MainWindow::runFactorial()
{
    runProgram(factorial, sizeof(factorial), "Factorial Program", 0, 23);
}

QString MainWindow::formatBinaryDisplay(int value)
{
    QString result;
    for (int i = 8-1; i >= 0; i--) {
        if ((value & (unsigned long long)(1ULL << i)) != 0) {
            result += "●";
        } else {
            result += "○";
        }
    }

    return result;
}

void MainWindow::onIpChange(int newIP)
{
    ui->ip->setText(formatBinaryDisplay(newIP));
}

void MainWindow::onSpChange(int newSP)
{
    ui->sp->setText(formatBinaryDisplay(newSP));
}

void MainWindow::onCallSpChange(int newSP)
{
    ui->callSp->setText(formatBinaryDisplay(newSP));
}

void MainWindow::onOpcodeChange(int newOP)
{
    ui->opcode->setText(formatBinaryDisplay(newOP));
}

void MainWindow::highlightCurrentLine(int currentIP)
{
    if (programLines.isEmpty() || currentIP < 0) {
        return;
    }
    
    // Find which line corresponds to the current IP
    int highlightLineIndex = -1;
    for (int i = 0; i < lineAddresses.size(); i++) {
        if (lineAddresses[i] == currentIP) {
            highlightLineIndex = i;
            break;
        }
    }
    
    if (highlightLineIndex == -1) {
        return; // IP doesn't match any instruction start
    }
    
    // Rebuild the listing with highlighting
    QString listing = QString();
    
    for (int i = 0; i < programLines.size(); i++) {
        if (i == highlightLineIndex) {
            // Highlight current line with background color
            listing += QString("> %1\n").arg(programLines[i]);
        } else {
            listing += QString("    %1\n").arg(programLines[i]);
        }
    }
    
    ui->programListing->setPlainText(listing);
}

void MainWindow::updateWindowTitle()
{
    QString status;
    if (isPaused) {
        status = "Paused";
    } else if (isRunning) {
        status = "Running";
    } else {
        status = "Halted";
    }
    setWindowTitle(QString("VM - %1 [%2]").arg(currentProgramName, status));
}

void MainWindow::onVmFinished()
{
    isRunning = false;
    isPaused = false;
    updateWindowTitle();
}

void MainWindow::onVmPaused(bool paused)
{
    isPaused = paused;
    updateWindowTitle();
    
    // Update pause button state
    ui->actionPause->setChecked(paused);
}

void MainWindow::onRunAction()
{
    // Always restart the VM from the beginning
    if (lastCode && lastCodeSize > 0) {
        // Stop current VM if running
        if (vm && isRunning) {
            vm->halt();
            vm = nullptr;
        }
        
        // Restart the last program
        runProgram(lastCode, lastCodeSize, lastProgramName, lastNglobals, lastIp);
    }
}

void MainWindow::onPauseAction()
{
    if (vm && isRunning) {
        if (vm->getPaused()) {
            // VM is paused, resume it
            vm->resume();
        } else {
            // VM is running, pause it
            vm->pause();
        }
    } else {
        // If VM is not running, ensure pause button is unchecked
        ui->actionPause->setChecked(false);
    }
}

void MainWindow::onHaltAction()
{
    if (vm && isRunning) {
        vm->halt();
    }
}
