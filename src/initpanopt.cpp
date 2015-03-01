#include <initpanopt.h>
#include <linalg/linalgparameterhandler.h>
#include <simplex/simplexparameterhandler.h>
#include <linalg/indexeddensevector.h>
#include <linalg/densevector.h>
#include <linalg/sparsevector.h>
#include <debug.h>
#include <iostream>
#include <cstdio>
#include <utils/platform.h>
#include <utils/thread.h>
#include <utils/stacktrace.h>

using namespace std;


// architectures
#ifdef PLATFORM_X86
#include <utils/arch/x86.h>
#else
#include <utils/arch/unknown.h>
#endif

//static const InitPanOpt & instance = InitPanOpt::getInstance();

thread_local InitPanOpt * InitPanOpt::sm_instance;
thread_local ArchitectureInterface * InitPanOpt::sm_architecture;

InitPanOpt::InitPanOpt() {

}

InitPanOpt::InitPanOpt(const InitPanOpt & orig) {
    __UNUSED(orig);
}

InitPanOpt &InitPanOpt::getInstance() {
    return *sm_instance;
}

void InitPanOpt::init() {
#ifndef CLASSIC_NEW_DELETE
    MemoryManager::sm_smallStacks = 0;

    MemoryManager::sm_largeStacks = 0;

    Pool::sm_head = 0;

    MemoryManager::sm_initialized = false;


    MemoryManager::init();
#endif

    SET_FPU_TO_64();

    D::init();
    sm_instance = new InitPanOpt();

    ThreadSupervisor::_globalInit();
    SimplexParameterHandler::_globalInit();
    LinalgParameterHandler::_globalInit();
    Numerical::_globalInit();
    IndexedDenseVector::_globalInit();
    DenseVector::_globalInit();
    SparseVector::_globalInit();
    GeneralMessageHandler::_globalInit();

#ifdef PLATFORM_X86
    sm_architecture = new ArchitectureX86;
#else
    sm_architecture = new ArchitectureUnknown;
#endif
    sm_architecture->loadParameters();
    sm_architecture->detect();

}

void InitPanOpt::threadInit() {

    SET_FPU_TO_64();

    IndexedDenseVector::_globalInit();
    DenseVector::_globalInit();
    SparseVector::_globalInit();

}

void InitPanOpt::release()
{
    ThreadSupervisor::_globalRelease();

    SparseVector::_globalRelease();

    D::release();

    LinalgParameterHandler::_globalRelease();
    SimplexParameterHandler::_globalRelease();

    delete sm_instance;
    sm_instance = nullptr;

    delete sm_architecture;
    sm_architecture = nullptr;

#ifndef CLASSIC_NEW_DELETE
    MemoryManager::release();
#endif
}

void InitPanOpt::threadRelease() {

    SET_FPU_TO_64();

    SparseVector::_globalRelease();

}

const ArchitectureInterface & InitPanOpt::getArchitecture() const {
    return *sm_architecture;
}

__attribute__((constructor))
void initPanOpt() {
    //initSignalHandler();
    setbuf(stdout, 0);
    printf("INIT PanOpt!\n");

    InitPanOpt::init();

}

__attribute__((destructor))
static void releasePanOpt() {
    std::cout << "RELEASE PanOpt" << std::endl;

    InitPanOpt::release();
}
