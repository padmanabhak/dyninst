#include "dwarf/h/dwarfResult.h"
#include "dynutil/h/VariableLocation.h"
#include "dwarf/h/dwarfFrameParser.h"
#include "dynutil/h/ProcReader.h"
#include "dynutil/h/dyntypes.h"
#include "dynutil/h/dyn_regs.h"
#include "common/h/Types.h"

using namespace Dyninst;
using namespace Dwarf;

#define CHECK_OPER(n) if (operands.size() < n) { error = true; break; }

void SymbolicDwarfResult::pushReg(MachRegister reg) {
   if (var.stClass != storageUnset) { error = true; }
   var.stClass = storageReg;
   var.refClass = storageNoRef;
   var.frameOffset = 0;
   var.mr_reg = reg;
}

void SymbolicDwarfResult::readReg(MachRegister reg) {
   if (var.stClass != storageUnset) { error = true; }
   var.stClass = storageRegOffset;
   var.refClass = storageNoRef;
   // frameOffset will be set with an add operation
   var.frameOffset = 0;
   var.mr_reg = reg;
}

void SymbolicDwarfResult::pushUnsignedVal(MachRegisterVal val) {
   if (var.stClass == storageUnset) {
      // No register, so default to StorageAddr
      var.stClass = storageAddr;
   }

   operands.push(val);
}

void SymbolicDwarfResult::pushSignedVal(MachRegisterVal val) {
   operands.push(val);
}

void SymbolicDwarfResult::pushOp(Operator op) {
   // This is "fill in as we see examples" code. 
   // Right now, the only use I know of is add. 
   switch (op) {
      case Add:
         CHECK_OPER(1);
         if (var.stClass == storageUnset) { error = true; }
         var.frameOffset += operands.top(); 
         operands.pop();
         break;
      default:
         error = true;
   }         
}

void SymbolicDwarfResult::pushOp(Operator, 
                                   unsigned) {
   error = true;
}

void SymbolicDwarfResult::pushFrameBase() {
   readReg(FrameBase);
}

void SymbolicDwarfResult::pushCFA() {
   readReg(CFA);
}

VariableLocation &SymbolicDwarfResult::val() {
   if (!operands.empty()) {
      var.frameOffset = operands.top(); 
      operands.pop();
   }
   return var;
}

void ConcreteDwarfResult::pushReg(MachRegister) {
   // I don't believe this is legal
   error = true;
}

void ConcreteDwarfResult::readReg(MachRegister reg) {
   Dyninst::MachRegisterVal v;
   if (!reader->GetReg(reg, v)) error = true;
   
   push(v);
}

void ConcreteDwarfResult::pushUnsignedVal(MachRegisterVal v) {
   // Someday this will matter...
   push(v);
}

void ConcreteDwarfResult::pushSignedVal(MachRegisterVal v) {
   // Someday this will matter...
   push(v);
}


void ConcreteDwarfResult::pushOp(Operator op) {
   MachRegisterVal v;
   MachRegisterVal first;
   MachRegisterVal second;

   switch (op) {
      case Add:
         CHECK_OPER(2);
         v = peek(1) + peek(0);
         popRange(0, 1);
         push(v);
         break;
      case Sub:
         CHECK_OPER(2);
         v = peek(1) - peek(0);
         popRange(0, 1);
         push(v);
         break;
      case Mul:
         CHECK_OPER(2);
         v = peek(1) * peek(0);
         popRange(0, 1);
         push(v);
         break;
      case Div:
         CHECK_OPER(2);
         if (peek(0) == 0) { error = true; break; }
         v = peek(1) / peek(0);
         popRange(0, 1);
         push(v);
         break;
      case Mod:
         CHECK_OPER(2);
         if (peek(0) == 0) { error = true; break; }
         v = peek(1) % peek(0);
         popRange(0, 1);
         push(v);
         break;
      case And:
         CHECK_OPER(2);
         v = peek(1) & peek(0);
         popRange(0, 1);
         push(v);
         break;
      case Or:
         CHECK_OPER(2);
         v = peek(1) | peek(0);
         popRange(0, 1);
         push(v);
         break; 
      case Not:
         CHECK_OPER(1);
         v = ~peek(0);
         pop(0);
         push(v);
         break;
      case Xor:
         CHECK_OPER(2);
         v = peek(0) ^ peek(0);
         popRange(0, 1);
         push(v);
         break;
      case Abs:
         CHECK_OPER(1);
         v = ::abs((long) peek(0));
         pop(0);
         push(v);
         break;
      case Shl:
         CHECK_OPER(2);
         v = peek(1) << peek(0);
         popRange(0, 1);
         push(v);
         break;
      case Shr:
         CHECK_OPER(2);
         v = ((unsigned long) peek(1)) >> ((unsigned long) peek(0));
         popRange(0, 1);
         push(v);
         break;
      case ShrArith:
         CHECK_OPER(2);
         v = ((long) peek(1)) + ((long) peek(0));
         popRange(0, 1);
         push(v);
         break;
      case GE:
         CHECK_OPER(2);
         second = peek(1);
         first = peek(0);
         popRange(0, 1);
         push((second >= first) ? 1 : 0);
         break;
      case LE:
         CHECK_OPER(2);
         second = peek(1);
         first = peek(0);
         popRange(0, 1);
         push((second <= first) ? 1 : 0);
         break;
      case GT:
         CHECK_OPER(2);
         second = peek(1);
         first = peek(0);
         popRange(0, 1);
         push((second > first) ? 1 : 0);
         break;
      case LT:
         CHECK_OPER(2);
         second = peek(1);
         first = peek(0);
         popRange(0, 1);
         push((second < first) ? 1 : 0);
         break;
      case Eq:
         CHECK_OPER(2);
         second = peek(1);
         first = peek(0);
         popRange(0, 1);
         push((second == first) ? 1 : 0);
         break;
      case Neq:
         CHECK_OPER(2);
         second = peek(1);
         first = peek(0);
         popRange(0, 1);
         push((second != first) ? 1 : 0);
         break;
      case Deref:         
      case Pick:
      case Drop:
      default:
         // 2 argument
         error = true;
         break;
   }
}

void ConcreteDwarfResult::pushOp(Operator op, unsigned ref) {
   switch (op) {
      case Deref: {
         CHECK_OPER(1);
         MachRegisterVal v;
         switch(ref) {
            case 1: {
               unsigned char c;
               if (!reader->ReadMem(peek(0), &c, sizeof(c))) error = true;
               v = c;
               break;
            }
            case 2: {
               unsigned short s;
               if (!reader->ReadMem(peek(0), &s, sizeof(s))) error = true;
               v = s;
               break;
            }
            case 4: {
               uint32_t u;
               if (!reader->ReadMem(peek(0), &u, sizeof(u))) error = true;
               v = u;
               break;
            }
            case 8: {
               uint64_t u;
               if (!reader->ReadMem(peek(0), &u, sizeof(u))) error = true;
               v = u;
               break;
            }
            default:
               error = true;
               v = 0;
               break;
         }
         push(v);
         break;
      }
      case Pick:
         CHECK_OPER(ref);
         push(peek(ref));
         break;
      case Drop:
         CHECK_OPER(ref);
         pop(ref);
         break;
      default:
         error = true;
         break;
   }
}

void ConcreteDwarfResult::pushFrameBase() {
   error = true;
}

void ConcreteDwarfResult::pushCFA() {
   DwarfFrameParser::Ptr cfaParser = DwarfFrameParser::create(dbg, arch);
   MachRegisterVal cfa;
   FrameErrors_t err;
   if (!cfaParser->getRegValueAtFrame(pc, 
                                      CFA, 
                                      cfa, 
                                      reader,
                                      err)) error = true;
   pushUnsignedVal(cfa);
}

MachRegisterVal ConcreteDwarfResult::peek(int index) {
   return operands[operands.size() - (index + 1)];
}

void ConcreteDwarfResult::pop(int num) {
   operands.erase(operands.begin() + (operands.size() - (num + 1)));
}

void ConcreteDwarfResult::popRange(int start, int end) {
   std::vector<MachRegisterVal>::iterator b, e;
   if (start > end) {
      b = operands.begin() + (operands.size() - (start + 1));
      e = operands.begin() + (operands.size() - end);
   }
   else {
      b = operands.begin() + (operands.size() - (end + 1));
      e = operands.begin() + (operands.size() - start);
   }
   operands.erase(b, e);
}

void ConcreteDwarfResult::push(MachRegisterVal v) {
   operands.push_back(v);
}

bool ConcreteDwarfResult::eval(MachRegisterVal &v) {
   if (err()) return false;
   v = val();
   return true;
}

MachRegisterVal ConcreteDwarfResult::val() {
   return operands.back();
}