//===-- SIPropagateImmReads.cpp - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// TODO: Add full description
//
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "AMDGPUUtil.h"
#include "AMDILMachineFunctionInfo.h"
#include "SIInstrInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

using namespace llvm;

namespace {
  class SIPropagateImmReadsPass : public MachineFunctionPass {

  private:
    static char ID;
    TargetMachine &TM;

  public:
    SIPropagateImmReadsPass(TargetMachine &tm) :
      MachineFunctionPass(ID), TM(tm) { }

    virtual bool runOnMachineFunction(MachineFunction &MF);
  };
} /* End anonymous namespace */

char SIPropagateImmReadsPass::ID = 0;

FunctionPass *llvm::createSIPropagateImmReadsPass(TargetMachine &tm) {
  return new SIPropagateImmReadsPass(tm);
}

bool SIPropagateImmReadsPass::runOnMachineFunction(MachineFunction &MF)
{
  AMDILMachineFunctionInfo * MFI = MF.getInfo<AMDILMachineFunctionInfo>();
  const SIInstrInfo * TII = static_cast<const SIInstrInfo*>(TM.getInstrInfo());

  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    for (MachineBasicBlock::iterator I = MBB.begin(), Next = llvm::next(I);
         I != MBB.end(); I = Next, Next = llvm::next(I)) {
      MachineInstr &MI = *I;

      switch (MI.getOpcode()) {
      case AMDIL::LOADCONST_f32:
      case AMDIL::LOADCONST_i32:
        break;
      default:
        continue;
      }

      /* XXX: Create and use S_MOV_IMM for SREGs */
      BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(AMDIL::V_MOV_IMM))
          .addOperand(MI.getOperand(0))
          .addOperand(MI.getOperand(1));

      MI.eraseFromParent();
    }
  }
}
