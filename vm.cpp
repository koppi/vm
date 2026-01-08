#include <QDebug>

#include "vm.h"

typedef struct {
    char name[8];
    int nargs;
} VM_INSTRUCTION;

static VM_INSTRUCTION vm_instructions[] = {
    { "noop",   0 },    // 0
    { "iadd",   0 },    // 1
    { "isub",   0 },    // 2
    { "imul",   0 },    // 3
    { "ilt",    0 },    // 4
    { "ieq",    0 },    // 5
    { "br",     1 },    // 7
    { "brt",    1 },    // 8
    { "brf",    1 },    // 9
    { "iconst", 1 },    // 10
    { "load",   1 },
    { "gload",  1 },
    { "store",  1 },
    { "gstore", 1 },
    { "print",  0 },
    { "pop",    0 },
    { "call",   3 },
    { "ret",    0 },
    { "halt",   0 }
};

VM::VM(int *code, int code_size, int nglobals, int startip, QObject *parent) : QThread(parent), startip(startip)
{
    init(code, code_size, nglobals);
}

void VM::init(int *code, int code_size, int nglobals)
{
    this->code = code;
    this->code_size = code_size;
    this->globals = (int *)calloc(nglobals, sizeof(int));
    this->nglobals = nglobals;
    
    // Initialize stack and pause control
    this->isPaused = false;
    this->shouldHalt = false;
}

VM::~VM()
{
    free(this->globals);
}

void VM::context_init(Context *ctx, int ip, int nlocals) {
    if ( nlocals>DEFAULT_NUM_LOCALS ) {
        fprintf(stderr, "too many locals requested: %d\n", nlocals);
    }
    ctx->returnip = ip;
}

void VM::run()
{
    exec(startip, true);
}

void VM::exec(int startip, bool trace)
{
    // registers
    int ip;         // instruction pointer register
    int sp;         // stack pointer register
    int callsp;     // call stack pointer register

    int a = 0;
    int b = 0;
    int addr = 0;
    int offset = 0;

    ip = startip;
    sp = -1;
    callsp = -1;
    
    // Emit initial register values
    emit ipChanged(ip);
    emit spChanged(sp);
    emit callSpChanged(callsp);
    
    // Check if starting IP is valid
    if (ip < 0 || ip >= this->code_size) {
        fprintf(stderr, "Invalid starting IP: %d (code size: %d)\n", ip, this->code_size);
        return;
    }
    
    int opcode = this->code[ip];

    // Initialize memory display at start
    if (trace) print_data(this->globals, this->nglobals);

    while (opcode != HALT && ip >= 0 && ip < this->code_size && !this->shouldHalt) {

        // Check for pause state - wait while paused
        if (this->isPaused) {
            msleep(1000); // Wait while paused
            continue;
        }
        
        if (trace) print_instr(this->code, ip);

        ip++; //jump to next instruction or to operand
        if (ip < this->code_size) emit ipChanged(ip);

        msleep(250); // from QThread

        switch (opcode) {
        case IADD:
            b = this->stack[sp--];           // 2nd opnd at top of stack
            a = this->stack[sp--];           // 1st opnd 1 below top
            this->stack[++sp] = a + b;       // push result
            emit spChanged(sp);
            break;
        case ISUB:
            b = this->stack[sp--];
            a = this->stack[sp--];
            this->stack[++sp] = a - b;
            emit spChanged(sp);
            break;
        case IMUL:
            b = this->stack[sp--];
            a = this->stack[sp--];
            this->stack[++sp] = a * b;
            emit spChanged(sp);
            break;
        case ILT:
            b = this->stack[sp--];
            a = this->stack[sp--];
            this->stack[++sp] = (a < b) ? true : false;
            emit spChanged(sp);
            break;
        case IEQ:
            b = this->stack[sp--];
            a = this->stack[sp--];
            this->stack[++sp] = (a == b) ? true : false;
            emit spChanged(sp);
            break;
        case BR:
            ip = this->code[ip];
            emit ipChanged(ip);
            break;
        case BRT:
            addr = this->code[ip++];
            if (this->stack[sp--] == true) {
                ip = addr;
                emit ipChanged(ip);
                emit spChanged(sp);
            }
            break;
        case BRF:
            addr = this->code[ip++];
            if (this->stack[sp--] == false) {
                ip = addr;
                emit ipChanged(ip);
                emit spChanged(sp);
            }
            break;
        case ICONST:
            this->stack[++sp] = this->code[ip++];  // push operand
            emit ipChanged(ip);
            emit spChanged(sp);
            break;
        case LOAD: // load local or arg
            offset = this->code[ip++];
            this->stack[++sp] = this->call_stack[callsp].locals[offset];
            emit ipChanged(ip);
            emit spChanged(sp);
            break;
        case GLOAD: // load from global memory
            addr = this->code[ip++];
            this->stack[++sp] = this->globals[addr];
            emit ipChanged(ip);
            emit spChanged(sp);
            if (trace) print_data(this->globals, this->nglobals);
            break;
        case STORE:
            offset = this->code[ip++];
            this->call_stack[callsp].locals[offset] = this->stack[sp--];
            emit ipChanged(ip);
            emit spChanged(sp);
            break;
        case GSTORE:
            addr = this->code[ip++];
            this->globals[addr] = this->stack[sp--];
            emit ipChanged(ip);
            emit spChanged(sp);
            if (trace) print_data(this->globals, this->nglobals);
            break;
        case PRINT:
            //printf("%d\n", this->stack[sp--]);
            emit hasStdout(QString("%1").arg(this->stack[sp--]));
            emit spChanged(sp);
            break;
        case POP:
            --sp;
            emit spChanged(sp);
            break;
        case RET:
            ip = this->call_stack[callsp].returnip;
            callsp--; // pop context
            emit ipChanged(ip);
            emit callSpChanged(ip);
            break;
        default:
            printf("invalid opcode: %d at ip=%d\n", opcode, (ip - 1));
            exit(1);
            break;
        case CALL:
            {
                // expects all args on stack
                addr = this->code[ip++];			// index of target function
                int nargs = this->code[ip++]; 	// how many args got pushed
                int nlocals = this->code[ip++]; 	// how many locals to allocate
                ++callsp; // bump stack pointer to reveal space for this call
                context_init(&this->call_stack[callsp], ip, nargs+nlocals);
                // copy args into new context
                for (int i=0; i<nargs; i++) {
                    this->call_stack[callsp].locals[i] = this->stack[sp-i];
                }
                sp -= nargs;
                ip = addr;		// jump to function
                emit callSpChanged(ip);
                emit ipChanged(ip);
                emit spChanged(sp);
                if (trace) print_data(this->globals, this->nglobals);
                break;
            }
        case HALT:
            // Halt execution - loop will terminate due to condition check
            if (trace) emit hasInstruction("HALT: Program execution terminated");
            break;
        }
        if (trace) {
            print_stack(this->stack, sp);
            // Update memory display periodically to show current state
			print_data(this->globals, this->nglobals);
        }
        opcode = this->code[ip];
        emit opcodeChanged(opcode);
    }
    if (trace) print_data(this->globals, this->nglobals);
}

void VM::print_instr(int *code, int ip)
{
    int opcode = code[ip];
    
    // Check if opcode is valid
    if (opcode < 0 || opcode >= static_cast<int>(sizeof(vm_instructions) / sizeof(VM_INSTRUCTION))) {
        emit hasInstruction(QString("%1:  INVALID_OPCODE_%2").arg(ip, 4, 10, QLatin1Char('0')).arg(opcode));
        return;
    }
    
    VM_INSTRUCTION *inst = &vm_instructions[opcode];
    QString tmp;
    switch (inst->nargs) {
    case 0:
        tmp = QString("%1:  %2").arg(ip, 4, 10, QLatin1Char('0')).arg(inst->name, -20);
        break;
    case 1:
        tmp = QString("%1:  %2%3").arg(ip, 4, 10, QLatin1Char('0')).arg(inst->name, -10).arg(code[ip + 1], -10);
        break;
    case 2:
        tmp = QString("%1:  %2%3,%4").arg(ip, 4, 10, QLatin1Char('0')).arg(inst->name, -10).arg(code[ip + 1]).arg(code[ip + 2], 10);
        break;
    case 3:
        tmp = QString("%1:  %2%3,%4,%5").arg(ip, 4, 10, QLatin1Char('0')).arg(inst->name, -10).arg(code[ip + 1]).arg(code[ip + 2]).arg(code[ip + 3], -6);
        break;
    }
    emit hasInstruction(tmp);
}

void VM::print_stack(int *stack, int count)
{
    QString tmp, tmp2;
    for (int i = 0; i <= count; i++) {
        tmp = QString(" %1").arg(stack[i]);
        tmp2 = tmp2 + tmp;
    }
    emit hasStack(tmp2);
}

void VM::print_data(int *globals, int count)
{
    QString tmp, tmp2;
    for (int i = 0; i < count; i++) {
        tmp = QString("%1: %2\n").arg(i, 4, 10, QLatin1Char('0')).arg(globals[i]);
        tmp2 = tmp2 + tmp;
    }
    emit hasMemory(QString("%1").arg(tmp2));
}

void VM::pause()
{
    this->isPaused = true;
    emit pausedChanged(true);
}

void VM::halt()
{
    this->shouldHalt = true;
}

void VM::resume()
{
    this->isPaused = false;
    this->shouldHalt = false;
    emit pausedChanged(false);
}

bool VM::getPaused() const
{
    return this->isPaused;
}
