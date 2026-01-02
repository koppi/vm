#ifndef VM_H
#define VM_H

#include <QThread>

#define DEFAULT_STACK_SIZE      1000
#define DEFAULT_CALL_STACK_SIZE 100
#define DEFAULT_NUM_LOCALS      10

typedef struct {
    int returnip;
    int locals[DEFAULT_NUM_LOCALS];
} Context;

class VM : public QThread
{
    Q_OBJECT
public:
    explicit VM(int *code, int code_size, int nglobals, int startip = 0, QObject *parent = nullptr);
    ~VM();

    void run() override;
    
    // Control methods
    void pause();
    void halt();
    void resume();
    bool getPaused() const;

    typedef enum {
        NOOP    = 0,
        IADD    = 1,   // int add
        ISUB    = 2,
        IMUL    = 3,
        ILT     = 4,   // int less than
        IEQ     = 5,   // int equal
        BR      = 6,   // branch
        BRT     = 7,   // branch if true
        BRF     = 8,   // branch if true
        ICONST  = 9,   // push constant integer
        LOAD    = 10,  // load from local context
        GLOAD   = 11,  // load from global memory
        STORE   = 12,  // store in local context
        GSTORE  = 13,  // store in global memory
        PRINT   = 14,  // print stack top
        POP     = 15,  // throw away top of stack
        CALL    = 16,  // call function at address with nargs,nlocals
        RET     = 17,  // return value from function
        HALT    = 18
    } VM_CODE;

signals:
    void hasStdout(QString txt);
    void hasStack(QString txt);
    void hasMemory(QString txt);
    void hasInstruction(QString txt);
    void ipChanged(int newIp);
    void spChanged(int newSp);
    void callSpChanged(int newSp);
    void opcodeChanged(int opCode);
    void pausedChanged(bool paused);

public slots:

public:
    void exec(int startip, bool trace);
    void print_data(int *globals, int count);

    // global variable space
    int *globals;
    int nglobals;
    
    // pause control
    bool isPaused;
    bool shouldHalt;

protected:
    void init(int *code, int code_size, int nglobals);
    void print_instr(int *code, int ip);
    void print_stack(int *stack, int count);

    void context_init(Context *ctx, int ip, int nlocals);
private:
    int *code;
    int code_size;
    int startip;

    // Operand stack, grows upwards
    int stack[DEFAULT_STACK_SIZE];
    Context call_stack[DEFAULT_CALL_STACK_SIZE];
};

#endif // VM_H
