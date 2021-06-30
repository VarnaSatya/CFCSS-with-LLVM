//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm-c/ExecutionEngine.h"
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "hello"
STATISTIC(HelloCounter, "Counts number of functions greeted");
static LLVMContext Context;
//static IRBuilder<> Builder(Context);

//SINGLE PREDECESSOR

namespace {
        // Hello - The first implementation, without getAnalysisUsage.
        struct Hello : public FunctionPass {
                static char ID; // Pass identification, replacement for typeid
                Hello() : FunctionPass(ID) {}

                llvm::GlobalVariable* g;
                llvm::GlobalVariable* D;
                std::map<BasicBlock * , int> s;
                Function *checking;
                bool runOnFunction(Function &F) override
                {
                        ++HelloCounter;
                        Module *M=F.getParent();

                        if(!F.getName().compare("__ctt_error"))
                        {
                                return false;
                        }

                        int d,t;

                        g=CreateGlobalVariable(M,"G");
                        D=CreateGlobalVariable(M,"D");

                        errs().write_escaped(F.getName()) << '\n';
                        std::map<BasicBlock * , int> s;
                        int count=1;

                        BasicBlock *p;

                        checking = M->getFunction("__ctt_error");


                        for(BasicBlock &BB : F)
                        {
                                s[&BB]=count++;
                        }

                        count=1;

                        for (BasicBlock &BB : F)
                        {
                                IRBuilder<> IR(M->getContext());
                                t=0;
                                //errs() << "\nBasic block (name=" << BB.getName() << ") has "<< BB.size() << " instructions.\nPredecessors:\n";

                                Instruction* InsertPt = &BB.front();
                                IR.SetInsertPoint(InsertPt);

                                int predD=0;
                                p=BB.getUniquePredecessor();

                                if(p!=NULL)//single predecessor
                                        errs()<<p->getName()<<"\n";
                                else//multiple predecessors
                                {
                                        for (auto it = pred_begin(&BB), et = pred_end(&BB); it != et; ++it)
                                        {
                                                p = *it;
                                                StoreInst *StoreD=IR.CreateStore(IR.getInt32(s[p]),D);
                                                errs() <<p->getName() <<"\n";
                                                t++;
                                                break;
                                        }
                                }

                                d=s[p]^s[&BB];      //XOR between signature of current block and predecessor block

                                /*entry block*/
                                if(count==1)
                                {
                                        StoreInst *Store=IR.CreateStore(IR.getInt32(s[&BB]),g);
                                }
                                /*single predecessor*/
                                else if(t<1)
                                {
                                        Value *gNew=CreateXORdG(M,&BB.front(),BB,IR.getInt32(d));
                                        //Instruction* Inst=cast<Instruction>(gNew);
                                        //errs()<<Inst<<"\n";
                                        //CreateCallAndAssignG(M,Inst,BB,gNew);
                                        IR.CreateCall(checking, {gNew,IR.getInt32(s[&BB])});
                                        StoreInst *Store=IR.CreateStore(gNew,g);

                                }
                                /*multiple predecessor*/
                                else if(t==1)
                                {
                                        BasicBlock* pred;
                                        for (auto it = pred_begin(&BB), et = pred_end(&BB); it != et; ++it)
                                        {
                                                pred=*it;
                                                LoadInst* dCur = IR.CreateLoad(D);
                                                Value* dNew = IR.CreateXor(dCur, s[pred]);
                                                Value *gNew=CreateXORdG(M,&BB.front(),BB,IR.getInt32(d));
                                                Value *gN=IR.CreateXor(gNew,dNew);
                                                //Instruction* Inst=cast<Instruction>(gN);
                                                //CreateCallAndAssignG(M,Inst,BB,gN);
                                                IR.CreateCall(checking, {gN,IR.getInt32(s[&BB])});
                                                StoreInst *Store=IR.CreateStore(gN,g);
                                        }
                                }
                                                        count++;

                                errs() <<"\n\n";
                        }
                        return true;

                }

                llvm::GlobalVariable* CreateGlobalVariable(Module* M,std::string s)
                {
                        IRBuilder<> Builder(M->getContext());
                        M->getOrInsertGlobal(s, Builder.getInt32Ty());
                        llvm::GlobalVariable* g = M->getNamedGlobal(s);
                        g->setLinkage(llvm::GlobalValue::CommonLinkage);
                        g->setAlignment(Align(4));
                        g->setInitializer(Builder.getInt32(0));
                        return g;
                }

                Value* CreateXORdG(Module *M,Instruction *Inst,BasicBlock &BB,Value *d)
                {
                        IRBuilder<> IR(M->getContext());
                        IR.SetInsertPoint(Inst);
                        LoadInst* gCur = IR.CreateLoad(g);
                        Value* gNew = IR.CreateXor(gCur, d);
                        return gNew;
                }

                /*void CreateCallAndAssignG(Module *M,Instruction *Inst,BasicBlock &BB,Value *gN)
                {
                        IRBuilder<> IR(M->getContext());
                        IR.SetInsertPoint(Inst);
                        IR.CreateCall(checking, {gN,IR.getInt32(s[&BB])});
                        StoreInst *Store=IR.CreateStore(gN,g);
                }*/

        };
}


char Hello::ID = 0;
static RegisterPass<Hello> X("hello", "Hello World Pass");

namespace {
        // Hello2 - The second implementation with getAnalysisUsage implemented.
        struct Hello2 : public FunctionPass {
                static char ID; // Pass identification, replacement for typeid
                Hello2() : FunctionPass(ID) {}

                bool runOnFunction(Function &F) override {
                        ++HelloCounter;
                        errs() << "Hello: ";
                        errs().write_escaped(F.getName()) << '\n';
                        return false;
                }

                // We don't modify the program, so we preserve all analyses.
                void getAnalysisUsage(AnalysisUsage &AU) const override {
                        AU.setPreservesAll();
                }
        };
}

char Hello2::ID = 0;
static RegisterPass<Hello2>
Y("hello2","Hello World Pass (with getAnalysisUsage implemented)");
