/**
 * @file simplex.cpp
 */

#include <simplex/simplex.h>
#include <utils/parameterhandler.h>
#include <utils/exceptions.h>
#include <linalg/linalgparameterhandler.h>
#include <simplex/simplexparameterhandler.h>
#include <simplex/startingbasisfinder.h>
#include <simplex/pfibasis.h>
#include <simplex/lubasis.h>
#include <globals.h>
#include <simplex/basisheadbas.h>
#include <simplex/basisheadpanopt.h>
#include <simplex/checker.h>

#include <utils/thirdparty/prettyprint.h>

//#include <qd/qd_real.h>
//#include <qd/fpu.h>

#include <algorithm>

const static char * SOLUTION_INVERSION_TIMER_NAME = "Inversion time";
const static char * SOLUTION_COMPUTE_BASIC_SOLUTION_TIMER_NAME = "Compute basic solution time";
const static char * SOLUTION_COMPUTE_REDUCED_COSTS_TIMER_NAME = "Compute reduced costs time";
const static char * SOLUTION_COMPUTE_FEASIBILITY_TIMER_NAME = "Compute feasibility time";
const static char * SOLUTION_CHECK_FEASIBILITY_TIMER_NAME = "Check feasibility time";
const static char * SOLUTION_PRICE_TIMER_NAME = "Pricing time";
const static char * SOLUTION_SELECT_PIVOT_TIMER_NAME = "Pivot selection time";
const static char * SOLUTION_UPDATE_TIMER_NAME = "Update time";

const static char * EXPORT_PROBLEM_NAME = "export_problem_name";
const static char * EXPORT_SOLUTION = "export_solution";
const static char * EXPORT_FALLBACK = "export_fallback";
const static char * EXPORT_SINGULARITY = "export_singularity";
const static char * EXPORT_BAD_ITERATION = "export_bad_iteration";
const static char * EXPORT_DEGENERATION = "export_degeneration";
const static char * EXPORT_E_ABSOLUTE = "export_e_absolute";
const static char * EXPORT_E_RELATIVE = "export_e_relative";
const static char * EXPORT_E_FEASIBILITY = "export_e_feasibility";
const static char * EXPORT_E_OPTIMALITY = "export_e_optimality";
const static char * EXPORT_E_PIVOT = "export_e_pivot";
const static char * EXPORT_PIVOT_THRESHOLD = "export_pivot_threshold";
const static char * EXPORT_NONLINEAR_DUAL_PHASEI_FUNCTION = "export_nonlinear_dual_phaseI_function";
const static char * EXPORT_NONLINEAR_DUAL_PHASEII_FUNCTION = "export_nonlinear_dual_phaseII_function";
const static char * EXPORT_ENABLE_FAKE_FEASIBILITY = "export_enable_fake_feasibility";

Simplex::Simplex(SimplexController &simplexController):
    m_simplexController(simplexController),
    m_simplexModel(NULL),
    m_degenerate(false),
    m_variableStates(0,0),
    m_basicVariableFeasibilities(0,0),
    m_reducedCostFeasibilities(0,0),
    m_basicVariableValues(0),
    m_reducedCosts(0),
    m_objectiveValue(0),
    m_phaseIObjectiveValue(-Numerical::Infinity),
    m_feasible(false),
    m_baseChanged(false),
    //Parameter references
    m_enableExport(SimplexParameterHandler::getInstance().getBoolParameterValue("enable_export")),
    m_exportFilename(SimplexParameterHandler::getInstance().getStringParameterValue("export_filename")),
    m_exportType(SimplexParameterHandler::getInstance().getIntegerParameterValue("export_type")),
    m_saveFormat(SimplexParameterHandler::getInstance().getStringParameterValue("save_format")),
    m_saveIteration(SimplexParameterHandler::getInstance().getIntegerParameterValue("save_iteration")),
    m_savePeriodically(SimplexParameterHandler::getInstance().getIntegerParameterValue("save_periodically")),
    m_saveFilename(SimplexParameterHandler::getInstance().getStringParameterValue("save_filename")),
    m_loadFilename(SimplexParameterHandler::getInstance().getStringParameterValue("load_filename")),
    m_loadFormat(SimplexParameterHandler::getInstance().getStringParameterValue("load_format")),
    m_masterTolerance(0),
    m_toleranceStep(0),
    m_workingTolerance(0),
    //Measures
    m_referenceObjective(0),
    m_fallbacks(0),
    m_badIterations(0),
    m_degenerateIterations(0),
    //Modules
    m_startingBasisFinder(NULL),
    m_basis(NULL)
{
    m_basicVariableValues.setSparsityRatio(DENSE);
    m_reducedCosts.setSparsityRatio(DENSE);
}

Simplex::~Simplex() {

    if(m_simplexModel){
        delete m_simplexModel;
        m_simplexModel = 0;
    }
}

std::vector<IterationReportField> Simplex::getIterationReportFields(
        enum ITERATION_REPORT_FIELD_TYPE & type) const {
    std::vector<IterationReportField> result;

    switch (type) {
    case IterationReportProvider::IRF_START:
        break;

    case IterationReportProvider::IRF_ITERATION:
        break;

    case IterationReportProvider::IRF_SOLUTION:
    {
        IterationReportField inversionTimerField(SOLUTION_INVERSION_TIMER_NAME, 20, 1, IterationReportField::IRF_RIGHT,
                                                 IterationReportField::IRF_FLOAT, *this,
                                                 4, IterationReportField::IRF_FIXED);
        result.push_back(inversionTimerField);
        IterationReportField computeBasicSolutionTimerField(SOLUTION_COMPUTE_BASIC_SOLUTION_TIMER_NAME, 20, 1, IterationReportField::IRF_RIGHT,
                                                            IterationReportField::IRF_FLOAT, *this,
                                                            4, IterationReportField::IRF_FIXED);
        result.push_back(computeBasicSolutionTimerField);
        IterationReportField computeReducedCostsField(SOLUTION_COMPUTE_REDUCED_COSTS_TIMER_NAME, 20, 1, IterationReportField::IRF_RIGHT,
                                                      IterationReportField::IRF_FLOAT, *this,
                                                      4, IterationReportField::IRF_FIXED);
        result.push_back(computeReducedCostsField);
        IterationReportField computeFeasibilityTimerField(SOLUTION_COMPUTE_FEASIBILITY_TIMER_NAME, 20, 1, IterationReportField::IRF_RIGHT,
                                                          IterationReportField::IRF_FLOAT, *this,
                                                          4, IterationReportField::IRF_FIXED);
        result.push_back(computeFeasibilityTimerField);
        IterationReportField checkFeasibilityTimerField(SOLUTION_CHECK_FEASIBILITY_TIMER_NAME, 20, 1, IterationReportField::IRF_RIGHT,
                                                        IterationReportField::IRF_FLOAT, *this,
                                                        4, IterationReportField::IRF_FIXED);
        result.push_back(checkFeasibilityTimerField);
        IterationReportField priceTimerField(SOLUTION_PRICE_TIMER_NAME, 20, 1, IterationReportField::IRF_RIGHT,
                                             IterationReportField::IRF_FLOAT, *this,
                                             4, IterationReportField::IRF_FIXED);
        result.push_back(priceTimerField);
        IterationReportField selectpivotTimerField(SOLUTION_SELECT_PIVOT_TIMER_NAME, 20, 1, IterationReportField::IRF_RIGHT,
                                                   IterationReportField::IRF_FLOAT, *this,
                                                   4, IterationReportField::IRF_FIXED);
        result.push_back(selectpivotTimerField);
        IterationReportField updateTimerField(SOLUTION_UPDATE_TIMER_NAME, 20, 1, IterationReportField::IRF_RIGHT,
                                              IterationReportField::IRF_FLOAT, *this,
                                              4, IterationReportField::IRF_FIXED);
        result.push_back(updateTimerField);
        break;
    }

    case IterationReportProvider::IRF_EXPORT:
    {
        // Parameter study export set
        if(m_exportType == 0){
            result.push_back(IterationReportField(EXPORT_PROBLEM_NAME, 20, 0, IterationReportField::IRF_LEFT,
                                                  IterationReportField::IRF_STRING, *this));
           result.push_back(IterationReportField(EXPORT_SOLUTION,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_FLOAT, *this,
                                                  10, IterationReportField::IRF_SCIENTIFIC));
            result.push_back(IterationReportField(EXPORT_FALLBACK,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_INT, *this));
            result.push_back(IterationReportField(EXPORT_SINGULARITY,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_INT, *this));
            result.push_back(IterationReportField(EXPORT_BAD_ITERATION,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_INT, *this));
            result.push_back(IterationReportField(EXPORT_DEGENERATION,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_INT, *this));
            result.push_back(IterationReportField(EXPORT_E_ABSOLUTE,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_FLOAT, *this,
                                                  10, IterationReportField::IRF_SCIENTIFIC));
            result.push_back(IterationReportField(EXPORT_E_RELATIVE,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_FLOAT, *this,
                                                  10, IterationReportField::IRF_SCIENTIFIC));
            result.push_back(IterationReportField(EXPORT_E_FEASIBILITY, 20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_FLOAT, *this,
                                                  10, IterationReportField::IRF_SCIENTIFIC));
            result.push_back(IterationReportField(EXPORT_E_OPTIMALITY, 20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_FLOAT, *this,
                                                  10, IterationReportField::IRF_SCIENTIFIC));
            result.push_back(IterationReportField(EXPORT_E_PIVOT, 20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_FLOAT, *this,
                                                  10, IterationReportField::IRF_SCIENTIFIC));
            result.push_back(IterationReportField(EXPORT_PIVOT_THRESHOLD, 20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_FLOAT, *this,
                                                  10, IterationReportField::IRF_SCIENTIFIC));
            // Ratio test research set
        } else if (m_exportType == 1) {
            result.push_back(IterationReportField(EXPORT_PROBLEM_NAME, 20, 0, IterationReportField::IRF_LEFT,
                                                  IterationReportField::IRF_STRING, *this));
            result.push_back(IterationReportField(EXPORT_SOLUTION,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_FLOAT, *this,
                                                  10, IterationReportField::IRF_SCIENTIFIC));
            result.push_back(IterationReportField(EXPORT_FALLBACK,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_INT, *this));
            result.push_back(IterationReportField(EXPORT_SINGULARITY,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_INT, *this));
            result.push_back(IterationReportField(EXPORT_BAD_ITERATION,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_INT, *this));
            result.push_back(IterationReportField(EXPORT_DEGENERATION,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_INT, *this));
            result.push_back(IterationReportField(EXPORT_NONLINEAR_DUAL_PHASEI_FUNCTION,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_INT, *this));
            result.push_back(IterationReportField(EXPORT_NONLINEAR_DUAL_PHASEII_FUNCTION,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_INT, *this));
            result.push_back(IterationReportField(EXPORT_ENABLE_FAKE_FEASIBILITY,  20, 0, IterationReportField::IRF_RIGHT,
                                                  IterationReportField::IRF_INT, *this));
        } else {
            throw ParameterException("Invalid export type specified in the parameter file!");
        }
        break;
    }
    default:
        break;
    }

    return result;
}

Entry Simplex::getIterationEntry(const string &name, ITERATION_REPORT_FIELD_TYPE &type) const {
    Entry reply;
    reply.m_integer = 0;
    switch (type) {
    case IterationReportProvider::IRF_START:
        break;

    case IterationReportProvider::IRF_ITERATION:
        break;

    case IterationReportProvider::IRF_SOLUTION:
        if (name == SOLUTION_INVERSION_TIMER_NAME) {
            reply.m_double = m_inversionTimer.getCPUTotalElapsed();
        } else if (name == SOLUTION_COMPUTE_BASIC_SOLUTION_TIMER_NAME) {
            reply.m_double = m_computeBasicSolutionTimer.getCPUTotalElapsed();
        } else if (name == SOLUTION_COMPUTE_REDUCED_COSTS_TIMER_NAME) {
            reply.m_double = m_computeReducedCostsTimer.getCPUTotalElapsed();
        } else if (name == SOLUTION_COMPUTE_FEASIBILITY_TIMER_NAME) {
            reply.m_double = m_computeFeasibilityTimer.getCPUTotalElapsed();
        } else if (name == SOLUTION_PRICE_TIMER_NAME) {
            reply.m_double = m_priceTimer.getCPUTotalElapsed();
        } else if (name == SOLUTION_SELECT_PIVOT_TIMER_NAME) {
            reply.m_double = m_selectPivotTimer.getCPUTotalElapsed();
        } else if (name == SOLUTION_UPDATE_TIMER_NAME) {
            reply.m_double = m_updateTimer.getCPUTotalElapsed();
        }
        break;

    case IterationReportProvider::IRF_EXPORT:
        if(name == EXPORT_PROBLEM_NAME){
            reply.m_string = new std::string(m_simplexModel->getName());
        } else if(name == EXPORT_SOLUTION){
            reply.m_double = m_objectiveValue;
        } else if(name == EXPORT_FALLBACK){
            reply.m_integer = m_fallbacks;
        } else if(name == EXPORT_SINGULARITY){
            reply.m_integer = m_basis->getSingularityCount();
        } else if(name == EXPORT_BAD_ITERATION){
            reply.m_integer = m_badIterations;
        } else if(name == EXPORT_DEGENERATION){
            reply.m_integer = m_degenerateIterations;
        }  else if(name == EXPORT_E_ABSOLUTE){
            reply.m_double = LinalgParameterHandler::getInstance().getDoubleParameterValue("e_absolute");
        } else if(name == EXPORT_E_RELATIVE){
            reply.m_double = LinalgParameterHandler::getInstance().getDoubleParameterValue("e_relative");
        } else if(name == EXPORT_E_FEASIBILITY){
            reply.m_double = SimplexParameterHandler::getInstance().getDoubleParameterValue("e_feasibility");
        } else if(name == EXPORT_E_OPTIMALITY){
            reply.m_double = SimplexParameterHandler::getInstance().getDoubleParameterValue("e_optimality");
        } else if(name == EXPORT_E_PIVOT){
            reply.m_double = SimplexParameterHandler::getInstance().getDoubleParameterValue("e_pivot");
        } else if(name == EXPORT_PIVOT_THRESHOLD){
            reply.m_double = SimplexParameterHandler::getInstance().getDoubleParameterValue("pivot_threshold");
        } else if(name == EXPORT_NONLINEAR_DUAL_PHASEI_FUNCTION){
            reply.m_integer = SimplexParameterHandler::getInstance().getIntegerParameterValue("nonlinear_dual_phaseI_function");
        } else if(name == EXPORT_NONLINEAR_DUAL_PHASEII_FUNCTION){
            reply.m_integer = SimplexParameterHandler::getInstance().getIntegerParameterValue("nonlinear_dual_phaseII_function");
        } else if(name == EXPORT_ENABLE_FAKE_FEASIBILITY){
            reply.m_integer = SimplexParameterHandler::getInstance().getIntegerParameterValue("enable_fake_feasibility");
        }
        break;

    default:
        break;
    }

    return reply;
}

void Simplex::setModel(const Model &model) {
    if(m_simplexModel){
        delete m_simplexModel;
    }
    unregisterMethodWithModel(this, model);
    m_simplexModel = new SimplexModel(model);
    registerMethodWithModel(this, model);

    //Init the data structures
    unsigned int rowCount = m_simplexModel->getRowCount();
    unsigned int columnCount = m_simplexModel->getColumnCount();
    m_basisHead.resize(rowCount, INVALID_POSITION);
    m_variableStates.init(rowCount + columnCount, VARIABLE_STATE_ENUM_LENGTH);
    m_basicVariableFeasibilities.init(rowCount, FEASIBILITY_ENUM_LENGTH);
    m_reducedCostFeasibilities.init(rowCount + columnCount, FEASIBILITY_ENUM_LENGTH);
    m_basicVariableValues.reInit(rowCount);
    m_reducedCosts.reInit(rowCount + columnCount);
    //TODO: Ezt ne kelljen ujra beallitani egy init miatt
    m_basicVariableValues.setSparsityRatio(DENSE);
    m_reducedCosts.setSparsityRatio(DENSE);

    if (SimplexParameterHandler::getInstance().getIntegerParameterValue("perturb_cost_vector") != 0){
        m_simplexModel->perturbCostVector();
    }
    if (SimplexParameterHandler::getInstance().getIntegerParameterValue("perturb_rhs") != 0){
        m_simplexModel->perturbRHS();
    }
    if (SimplexParameterHandler::getInstance().getIntegerParameterValue("shift_bounds") != 0){
        m_simplexModel->shiftBounds();
    }
}

void Simplex::setSimplexState(const Simplex & simplex)
{
    m_basisHead = simplex.m_basisHead;

    m_variableStates.clearAllPartitions();
    IndexList<const Numerical::Double*>::Iterator it;
    IndexList<const Numerical::Double*>::Iterator endit;

    //NONBASIC_AT_LB
    int partition = 1;
    simplex.m_variableStates.getIterators(&it,&endit,partition,1);
    for(;it != endit; it++){
        m_variableStates.insert(partition,it.getData(), &m_simplexModel->getVariable(it.getData()).getLowerBound());
    }
    partition++;
    //NONBASIC_AT_UB
    simplex.m_variableStates.getIterators(&it,&endit,partition,1);
    for(;it != endit; it++){
        m_variableStates.insert(partition,it.getData(), &m_simplexModel->getVariable(it.getData()).getUpperBound());
    }
    partition++;
    //NONBASIC_AT_FIXED
    simplex.m_variableStates.getIterators(&it,&endit,partition,1);
    for(;it != endit; it++){
        m_variableStates.insert(partition,it.getData(), &m_simplexModel->getVariable(it.getData()).getLowerBound());
    }
    partition++;
    //NONBASIC_AT_FREE
    simplex.m_variableStates.getIterators(&it,&endit,partition,1);
    for(;it != endit; it++){
        m_variableStates.insert(partition,it.getData(), &ZERO);
    }
    partition++;
    //BASIC
    simplex.m_variableStates.getIterators(&it,&endit,0,1);
    for(;it != endit; it++){
        m_variableStates.insert(0,it.getData(),&ZERO);
    }

    m_basicVariableValues = simplex.m_basicVariableValues;

}

void Simplex::iterate()
{
    computeWorkingTolerance();

    m_priceTimer.start();
    price();
    m_priceTimer.stop();

    m_selectPivotTimer.start();
    selectPivot();
    m_selectPivotTimer.stop();

    m_updateTimer.start();
    update();
    m_updateTimer.stop();
}

//void Simplex::constraintAdded()
//{
//}

//void Simplex::variableAdded()
//{
//}

void Simplex::saveBasisToFile(const char * fileName, BasisHeadIO * basisWriter, bool releaseWriter) {
    basisWriter->startWrting(fileName, m_simplexModel->getModel() );
    int variableIndex;
    unsigned int position = 0;

    //    LPINFO("basic variables: " << m_basicVariableValues );

    STL_FOREACH(std::vector<int>, m_basisHead, basisHeadIter) {
        variableIndex = *basisHeadIter;
        //        std::cout << variableIndex << " ";
        const Variable * variable = &m_simplexModel->getVariable(variableIndex);
        basisWriter->addBasicVariable( variable, position, variableIndex, m_basicVariableValues.at(position) );
        position++;
    }
    //    std::cout << std::endl;

    IndexList<const Numerical::Double *>::Iterator iter, iterEnd;

    m_variableStates.getIterators(&iter, &iterEnd, Simplex::NONBASIC_FIXED, 1);
    for (; iter != iterEnd; iter++) {
        variableIndex = iter.getData();
        const Variable * variable = &m_simplexModel->getVariable(variableIndex);
        // TODO: itt az erteket nem kell majd atadni
        basisWriter->addNonbasicVariable(variable, variableIndex, Simplex::NONBASIC_FIXED, variable->getLowerBound());
    }

    m_variableStates.getIterators(&iter, &iterEnd, Simplex::NONBASIC_AT_LB, 1);
    for (; iter != iterEnd; iter++) {
        variableIndex = iter.getData();
        const Variable * variable = &m_simplexModel->getVariable(variableIndex);
        // TODO: itt az erteket nem kell majd atadni
        basisWriter->addNonbasicVariable(variable, variableIndex, Simplex::NONBASIC_AT_LB, variable->getLowerBound());
    }

    m_variableStates.getIterators(&iter, &iterEnd, Simplex::NONBASIC_AT_UB, 1);
    for (; iter != iterEnd; iter++) {
        variableIndex = iter.getData();
        const Variable * variable = &m_simplexModel->getVariable(variableIndex);
        // TODO: itt az erteket nem kell majd atadni
        basisWriter->addNonbasicVariable(variable, variableIndex, Simplex::NONBASIC_AT_UB, variable->getUpperBound());
    }

    basisWriter->finishWriting();

    if (releaseWriter == true) {
        delete basisWriter;
    }


    // dump
    /*std::ofstream log("log_save.txt");

    log << "BASIS HEAD: ";
    unsigned int index;
    for (index = 0; index < m_basisHead.size(); index++) {
        log << m_basisHead[index] << " ";
    }
    log << endl;

    log << "VARIABLE STATES: " << endl;
    {
        IndexList<const Numerical::Double*>::Iterator iter, iterEnd;
        m_variableStates.getIterators(&iter, &iterEnd, BASIC);
        log << "BASIC: ";
        for (; iter != iterEnd; iter++) {
            log << "(" << iter.getData() << "; " << *iter.getAttached() << ") ";
        }
        log << endl;

        m_variableStates.getIterators(&iter, &iterEnd, NONBASIC_AT_LB);
        log << "NONBASIC_AT_LB: ";
        for (; iter != iterEnd; iter++) {
            log << "(" << iter.getData() << "; " << *iter.getAttached() << ") ";
        }
        log << endl;

        m_variableStates.getIterators(&iter, &iterEnd, NONBASIC_AT_UB);
        log << "NONBASIC_AT_UB: ";
        for (; iter != iterEnd; iter++) {
            log << "(" << iter.getData() << "; " << *iter.getAttached() << ") ";
        }
        log << endl;

        m_variableStates.getIterators(&iter, &iterEnd, NONBASIC_FIXED);
        log << "NONBASIC_FIXED: ";
        for (; iter != iterEnd; iter++) {
            log << "(" << iter.getData() << "; " << *iter.getAttached() << ") ";
        }
        log << endl;

        m_variableStates.getIterators(&iter, &iterEnd, NONBASIC_FREE);
        log << "NONBASIC_FREE: ";
        for (; iter != iterEnd; iter++) {
            log << "(" << iter.getData() << "; " << *iter.getAttached() << ") ";
        }
        log << endl;
    }
    log.close();*/
}

void Simplex::loadBasisFromFile(const char * fileName, BasisHeadIO * basisReader, bool releaseReader) {
    const unsigned int variableCount = m_simplexModel->getColumnCount() + m_simplexModel->getRowCount();
    basisReader->setBasisHead(&m_basisHead);
    basisReader->setVariableStateList(&m_variableStates);
    basisReader->startReading(fileName, m_simplexModel->getModel());
    unsigned int index;
    for (index = 0; index < variableCount; index++) {
        basisReader->addVariable(m_simplexModel->getVariable(index));
    }
    basisReader->finishReading();

    if (releaseReader) {
        delete basisReader;
    }

    m_variableStates.reversePartition(BASIC);
    m_variableStates.reversePartition(NONBASIC_AT_LB);
    m_variableStates.reversePartition(NONBASIC_AT_UB);
    m_variableStates.reversePartition(NONBASIC_FIXED);
    m_variableStates.reversePartition(NONBASIC_FREE);

    IndexList<const Numerical::Double*>::Iterator iter, iterEnd;
    m_variableStates.getIterators(&iter, &iterEnd, BASIC);
    for (; iter != iterEnd; iter++) {
        iter.setAttached( &m_basicVariableValues.at( iter.getData() ) );
    }
    m_variableStates.getIterators(&iter, &iterEnd, NONBASIC_AT_LB);
    for (; iter != iterEnd; iter++) {
        iter.setAttached( &m_simplexModel->getVariable( iter.getData() ).getLowerBound() );
    }

    m_variableStates.getIterators(&iter, &iterEnd, NONBASIC_AT_UB);
    for (; iter != iterEnd; iter++) {
        iter.setAttached( &m_simplexModel->getVariable( iter.getData() ).getUpperBound() );
    }

    m_variableStates.getIterators(&iter, &iterEnd, NONBASIC_FIXED);
    for (; iter != iterEnd; iter++) {
        iter.setAttached( &m_simplexModel->getVariable( iter.getData() ).getLowerBound() );
    }

    m_variableStates.getIterators(&iter, &iterEnd, NONBASIC_FREE);
    for (; iter != iterEnd; iter++) {
        iter.setAttached( &ZERO );
    }


    //    LPINFO("basis head: ");
    //    std::cout << std::dec;
    //    for (unsigned int i = 0; i < m_basisHead.size(); i++) {
    //        std::cout << m_basisHead[i] << " ";
    //    }
    //    std::cout << std::endl;

    // dump
    /* std::ofstream log("log_load.txt");

    log << "BASIS HEAD: ";

    for (index = 0; index < m_basisHead.size(); index++) {
        log << m_basisHead[index] << " ";
    }
    log << endl;
    log << "VARIABLE STATES: " << endl;
    {
        IndexList<const Numerical::Double*>::Iterator iter, iterEnd;
        m_variableStates.getIterators(&iter, &iterEnd, BASIC);
        log << "BASIC: ";
        for (; iter != iterEnd; iter++) {
            log << "(" << iter.getData() << "; " << *iter.getAttached() << ") ";
        }
        log << endl;

        m_variableStates.getIterators(&iter, &iterEnd, NONBASIC_AT_LB);
        log << "NONBASIC_AT_LB: ";
        for (; iter != iterEnd; iter++) {
            log << "(" << iter.getData() << "; " << *iter.getAttached() << ") ";
        }
        log << endl;

        m_variableStates.getIterators(&iter, &iterEnd, NONBASIC_AT_UB);
        log << "NONBASIC_AT_UB: ";
        for (; iter != iterEnd; iter++) {
            log << "(" << iter.getData() << "; " << *iter.getAttached() << ") ";
        }
        log << endl;

        m_variableStates.getIterators(&iter, &iterEnd, NONBASIC_FIXED);
        log << "NONBASIC_FIXED: ";
        for (; iter != iterEnd; iter++) {
            log << "(" << iter.getData() << "; " << *iter.getAttached() << ") ";
        }
        log << endl;

        m_variableStates.getIterators(&iter, &iterEnd, NONBASIC_FREE);
        log << "NONBASIC_FREE: ";
        for (; iter != iterEnd; iter++) {
            log << "(" << iter.getData() << "; " << *iter.getAttached() << ") ";
        }
        log << endl;
    }
    log.close();*/
}

void Simplex::findStartingBasis()
{
    StartingBasisFinder::STARTING_BASIS_STRATEGY startingBasisStratgy =
            (StartingBasisFinder::STARTING_BASIS_STRATEGY)
            SimplexParameterHandler::getInstance().getIntegerParameterValue("starting_basis_strategy");
    StartingBasisFinder::STARTING_NONBASIC_STATES startingNonbasicStates =
            (StartingBasisFinder::STARTING_NONBASIC_STATES)
            SimplexParameterHandler::getInstance().getIntegerParameterValue("starting_nonbasic_states");
    m_startingBasisFinder->findStartingBasis(startingBasisStratgy, startingNonbasicStates);

    if (m_pricing) {
        m_pricing->init();
    } else {
        // TODO: pricingcontroller!!!!!!
    }
}

void Simplex::initModules() {
    m_startingBasisFinder = new StartingBasisFinder(*m_simplexModel, &m_basisHead, &m_variableStates);
    int factorizationType = SimplexParameterHandler::getInstance().getIntegerParameterValue("factorization_type");
    if (factorizationType == 0){
        m_basis = new PfiBasis(*m_simplexModel, &m_basisHead, &m_variableStates, m_basicVariableValues);
    } else if (factorizationType == 1){
        m_basis = new LuBasis(*m_simplexModel, &m_basisHead, &m_variableStates, m_basicVariableValues);
    } else {
        LPERROR("Wrong parameter: factorization_type");
        throw ParameterException("Wrong factorization type parameter");
    }

    if(m_enableExport){
        m_iterationReport->addProviderForExport(*this);
    }

    initWorkingTolerance();
}

void Simplex::releaseModules() {
    if(m_startingBasisFinder){
        delete m_startingBasisFinder;
        m_startingBasisFinder = 0;
    }
    if(m_basis){
        delete m_basis;
        m_basis = 0;
    }
}

void Simplex::saveBasis(int iterationIndex)
{
    if ((iterationIndex == m_saveIteration) ||
            (m_savePeriodically != 0 && ((iterationIndex % m_savePeriodically) == 0) )){
        LPERROR("Saving basis "<<iterationIndex);
        stringstream numStream;
        numStream << iterationIndex;
        std::string saveFormat = m_saveFormat;
        std::string filename = m_saveFilename;
        filename.append("_").append(numStream.str()).append(".").append(m_saveFormat);
        std::transform(saveFormat.begin(),saveFormat.end(),saveFormat.begin(),::toupper);
        if(saveFormat.compare("BAS") == 0){
            saveBasisToFile(filename.c_str(), new BasisHeadBAS, true);
        } else if (saveFormat.compare("PBF") == 0){
            saveBasisToFile(filename.c_str(), new BasisHeadPanOpt, true);
        } else {
            throw ParameterException("Invalid save basis file format!");
        }
    }
}

void Simplex::loadBasis()
{
    std::string loadFormat = m_loadFormat;
    std::string filename = m_loadFilename;
    filename.append(".").append(m_loadFormat);
    //TODO: Exception when file does not exit
    std::transform(loadFormat.begin(),loadFormat.end(),loadFormat.begin(),::toupper);
    if(loadFormat.compare("BAS") == 0){
        loadBasisFromFile(filename.c_str(), new BasisHeadBAS, true);
    } else if (loadFormat.compare("PBF") == 0){
        loadBasisFromFile(filename.c_str(), new BasisHeadPanOpt, true);
    } else {
        throw ParameterException("Invalid load basis file format!");
    }
}

const std::vector<Numerical::Double> Simplex::getPrimalSolution() const {
    if(!(m_simplexModel->getModel().isPresolved())) {
        std::vector<Numerical::Double> result;
        unsigned int totalVariableCount = m_simplexModel->getColumnCount() + m_simplexModel->getRowCount();
        result.resize(totalVariableCount);
        if ( m_simplexModel->getModel().isScaled() ) {
            const std::vector<Numerical::Double> & columnMultipliers = m_simplexModel->getModel().getColumnMultipliers();
            for(unsigned int i = 0; i<totalVariableCount; i++) {
                result[i] = *(m_variableStates.getAttachedData(i)) / columnMultipliers[i];
            }
        } else {
            for(unsigned int i = 0; i<totalVariableCount; i++) {
                result[i] = *(m_variableStates.getAttachedData(i));
            }
        }
        return result;
    } else {
        unsigned int totalVariableCount = m_simplexModel->getColumnCount() + m_simplexModel->getRowCount();
        Vector structuralSolution(m_simplexModel->getColumnCount()+3, Vector::DENSE_VECTOR);
        structuralSolution.setNewNonzero(m_simplexModel->getColumnCount(), 1);
        Vector logicalSolution(m_simplexModel->getRowCount()+3, Vector::DENSE_VECTOR);
        logicalSolution.set(m_simplexModel->getColumnCount(), 1);
        structuralSolution.setSparsityRatio(0);
        logicalSolution.setSparsityRatio(0);
        if ( m_simplexModel->getModel().isScaled() ) {
            const std::vector<Numerical::Double> & columnMultipliers = m_simplexModel->getModel().getColumnMultipliers();
            for(unsigned int i = 0; i<m_simplexModel->getColumnCount(); i++) {
                structuralSolution.set(i, *(m_variableStates.getAttachedData(i)) / columnMultipliers[i]);
            }
            for(unsigned int i = m_simplexModel->getColumnCount(); i < totalVariableCount; i++) {
                logicalSolution.set(i - m_simplexModel->getColumnCount() + 1, *(m_variableStates.getAttachedData(i)) / columnMultipliers[i]);
            }
        } else {
            for(unsigned int i = 0; i<m_simplexModel->getColumnCount(); i++) {
                structuralSolution.set(i, *(m_variableStates.getAttachedData(i)));
            }
            for(unsigned int i = m_simplexModel->getColumnCount(); i < totalVariableCount; i++) {
                logicalSolution.set(i - m_simplexModel->getColumnCount(), *(m_variableStates.getAttachedData(i)));
            }
        }
        const std::vector<Vector*> * subsVectors = m_simplexModel->getModel().getSubstituteVectors();
        for(int i = subsVectors->size() - 1; i >= 0; i--) {
            Vector* currentSubstituteVector = (*subsVectors)[i];
            LPINFO("restoring variable to index " << currentSubstituteVector->at(currentSubstituteVector->length()-2));
            structuralSolution.insertElement(currentSubstituteVector->at(currentSubstituteVector->length()-2), structuralSolution.dotProduct(*currentSubstituteVector));
            totalVariableCount++;
        }
        std::vector<Numerical::Double> result;
        result.resize(totalVariableCount);
        for(unsigned i = 0; i < structuralSolution.length()-3; i++) {
            result[i] = structuralSolution.at(i);
        }
        for(unsigned i = structuralSolution.length()-3; i < totalVariableCount; i++) {
            result[i] = logicalSolution.at(i-structuralSolution.length()-1);
        }
        return result;
    }
}


const std::vector<Numerical::Double> Simplex::getDualSolution() const {
    return std::vector<Numerical::Double>();
    //TODO
}


void Simplex::reinvert() {
    releaseLocks();
    m_inversionTimer.start();
    m_basis->invert();
    m_inversionTimer.stop();

//    Checker::checkBasisWithFtran(*this);
//    Checker::checkBasisWithBtran(*this);
//    Checker::checkBasisWithNonbasicReducedCost(*this);
//    Checker::checkBasisWithReducedCost(*this);

    m_computeBasicSolutionTimer.start();
    computeBasicSolution();
    m_computeBasicSolutionTimer.stop();
    m_computeReducedCostsTimer.start();

//    if(!m_degenerate){
        computeReducedCosts();
//    }
    m_computeReducedCostsTimer.stop();
    m_computeFeasibilityTimer.start();
    computeFeasibility();
    m_computeFeasibilityTimer.stop();
}

void Simplex::computeBasicSolution() {
    m_basicVariableValues = m_simplexModel->getRhs();
    m_objectiveValue = - m_simplexModel->getCostConstant();

    const unsigned int columnCount = m_simplexModel->getColumnCount();
    //x_B=B^{-1}*(b-\SUM{U*x_U}-\SUM{L*x_L})
    IndexList<const Numerical::Double *>::Iterator it;
    IndexList<const Numerical::Double *>::Iterator itend;
    //This iterates through Simplex::NONBASIC_AT_LB, Simplex::NONBASIC_AT_UB and Simplex::NONBASIC_FIXED
    m_variableStates.getIterators(&it, &itend, Simplex::NONBASIC_AT_LB,3);

    for(; it != itend; it++) {
        if(*(it.getAttached()) != 0){
            if(it.getData() < columnCount){
                m_basicVariableValues.addVector(-1 * *(it.getAttached()), m_simplexModel->getMatrix().column(it.getData()), Numerical::ADD_ABS_REL);
                m_objectiveValue += m_simplexModel->getCostVector().at(it.getData()) * *(it.getAttached());
            } else {
                m_basicVariableValues.set(it.getData() - columnCount,
                                          Numerical::stableAdd(m_basicVariableValues.at(it.getData() - columnCount), - *(it.getAttached())));
//                                                          m_basicVariableValues.at(it.getData() - columnCount) - *(it.getAttached()));
            }
        }
    }

    //    This also sets the basic solution since the pointers of the basic variables point to the basic variable values vector
    //    Vector checkVector;

        m_basis->Ftran(m_basicVariableValues);

        /*m_basis->FtranCheck(m_basicVariableValues, checkVector);
        //LPINFO(m_basicVariableValues << checkVector);
        Numerical::Double norm1 = m_basicVariableValues.euclidNorm();

        double sum = 0.0;
        for (int i = 0; i < checkVector.length(); i++) {
            double a = m_basicVariableValues.at(i);
            double b = checkVector.at(i);
            if (a != 0 && b != 0) {
                double ratio = fabs(a) > fabs(b) ? a / b : b / a;
                sum += ratio * ratio;
            }
        }
        LPINFO("ratio norm: " << sqrt(sum));

        checkVector.addVector(-1, m_basicVariableValues);

        Numerical::Double norm2 = checkVector.euclidNorm();
        //LPINFO(checkVector);
        Numerical::Double normRatio = norm1 > norm2 ? norm1 / norm2 : norm2 / norm1;
        LPINFO("------ CHECK EUCLID NORM " << checkVector.euclidNorm() << "  " << norm1 << " " << norm2);
        LPINFO("ratio: " << normRatio << "   " << (normRatio - 1.0) << "  " << (normRatio - 1.0 >= 1e-5));



        if (normRatio - 1.0 >= 1e-4) {
            static int _counter = 0;
            _counter++;
            cout << "nagyobb " << _counter << endl;
            Timer timer;
            timer.start();
            sensitivityAnalysisRhs();
            timer.stop();
            cout << "time: " << timer.getTotalElapsed() << endl;
            cin.get();
            cin.get();
        }*/
    m_variableStates.getIterators(&it, &itend, Simplex::BASIC);
    for(; it != itend; it++) {
        if(it.getData() < columnCount){
            m_objectiveValue += m_simplexModel->getCostVector().at(it.getData()) * *(it.getAttached());
        }
    }
}

void Simplex::computeReducedCosts() {
    m_reducedCosts.clear();
    unsigned int rowCount = m_simplexModel->getRowCount();
    unsigned int columnCount = m_simplexModel->getColumnCount();

    //Get the c_B vector
    Vector simplexMultiplier(rowCount);
    simplexMultiplier.setSparsityRatio(DENSE);
    const Vector& costVector = m_simplexModel->getCostVector();
    for(unsigned int i = 0; i<m_basisHead.size(); i++){
        // TODO: majd a perturbacio miatt el kell tekinteni a feltetel elso feletol
        if(m_basisHead[i] < (int) columnCount && costVector.at(m_basisHead[i]) != 0.0){
            simplexMultiplier.setNewNonzero(i, costVector.at(m_basisHead[i]));
        }
    }
    //Compute simplex multiplier
    m_basis->Btran(simplexMultiplier);

    //For each variable
    for(unsigned int i = 0; i < rowCount + columnCount; i++) {
        if(m_variableStates.where(i) == Simplex::BASIC){
            //m_reducedCosts.set(i, 0.0);
            continue;
        }
        //Compute the dot product and the reduced cost
        Numerical::Double reducedCost;
        if(i < columnCount){
            reducedCost = Numerical::stableAdd(costVector.at(i), - simplexMultiplier.dotProduct(m_simplexModel->getMatrix().column(i),true,true));
        } else {
            reducedCost = -1 * simplexMultiplier.at(i - columnCount);
        }
        if(reducedCost != 0.0){
            m_reducedCosts.setNewNonzero(i, reducedCost);
        }
    }
}

Numerical::Double Simplex::sensitivityAnalysisRhs() const
{
    LPINFO("SENSITIVITY ANALYSIS");
    Numerical::Double lower = -Numerical::Infinity;
    Numerical::Double upper = Numerical::Infinity;
    Numerical::Double feasibilityTolerance =
            SimplexParameterHandler::getInstance().getDoubleParameterValue("e_feasibility") * 1000;

    unsigned int columnIndex;
    unsigned int rowIndex;
    const unsigned int rowCount = m_simplexModel->getMatrix().rowCount();
    const std::vector<Variable> & variables = m_simplexModel->getVariables();

    std::vector<Numerical::Double> lowerBoundDiffs(rowCount, -feasibilityTolerance);
    std::vector<Numerical::Double> upperBoundDiffs(rowCount, feasibilityTolerance);
    for (rowIndex = 0; rowIndex < rowCount; rowIndex++) {
        const unsigned int variableIndex = m_basisHead[rowIndex];
        const Numerical::Double lowerBound = variables[variableIndex].getLowerBound() - feasibilityTolerance;
        const Numerical::Double upperBound = variables[variableIndex].getUpperBound() + feasibilityTolerance;
        const Numerical::Double value = m_basicVariableValues.at(rowIndex);
        if (value >= lowerBound && value <= upperBound) {
            lowerBoundDiffs[rowIndex] = value - lowerBound;
            upperBoundDiffs[rowIndex] = upperBound - value;
        } else if (value < lowerBound) {
            lowerBoundDiffs[rowIndex] = Numerical::Infinity;
            upperBoundDiffs[rowIndex] = upperBound - value;
        } else if (value > upperBound) {
            lowerBoundDiffs[rowIndex] = value - lowerBound;
            upperBoundDiffs[rowIndex] = Numerical::Infinity;
        }
    }

    Vector beta(rowCount, Vector::DENSE_VECTOR);
    Numerical::Double sumMinLog = 0.0;
    Numerical::Double sumMaxLog = 0.0;
    Numerical::Double sumDiffLog = 0.0;
    for (columnIndex = 0; columnIndex < rowCount; columnIndex++) {
        lower = -Numerical::Infinity;
        upper = Numerical::Infinity;

        beta.clear();
        beta.setNewNonzero(columnIndex, 1.0);
        m_basis->Ftran(beta);

        Vector::NonzeroIterator betaIter = beta.beginNonzero();
        Vector::NonzeroIterator betaIterEnd = beta.endNonzero();
        for (; betaIter != betaIterEnd; betaIter++) {
            const unsigned int rowIndex = betaIter.getIndex();
            Numerical::Double ratio = -lowerBoundDiffs.at(rowIndex) / *betaIter;
            if (ratio > lower && *betaIter > 0.0) {
                lower = ratio;
            }
            if (ratio < upper && *betaIter < 0.0) {
                upper = ratio;
            }
            ratio = -upperBoundDiffs.at(rowIndex) / *betaIter;
            if (ratio > lower && *betaIter > 0.0) {
                lower = ratio;
            }
            if (ratio < upper && *betaIter < 0.0) {
                upper = ratio;
            }

        }

        const unsigned int variableIndex = m_basisHead[rowIndex];
        const Numerical::Double lowerBound = variables[variableIndex].getLowerBound() - feasibilityTolerance;
        const Numerical::Double upperBound = variables[variableIndex].getUpperBound() + feasibilityTolerance;
        const Numerical::Double value = m_basicVariableValues.at(rowIndex);

        if (value == lowerBound || value == upperBound) {
            //continue;
        }

       // LPINFO(lowerBound << " <= " << value << " <= " << upperBound);
       // LPINFO(columnIndex << ": " << lower << " <= delta <= " << upper << "   " << (upper - lower) << "   " <<
       //        log10(Numerical::fabs(lower)) << "  " << log10(Numerical::fabs(upper)));

        if (Numerical::fabs(lower) != Numerical::Infinity && lower != 0.0) {
            sumMinLog += log10(Numerical::fabs(lower));
        }
        if (Numerical::fabs(upper) != Numerical::Infinity && upper != 0.0) {
            sumMaxLog += log10(Numerical::fabs(upper));
        }
        if (Numerical::fabs(upper - lower) != Numerical::Infinity && upper != lower) {
            sumDiffLog += log10(Numerical::fabs(upper - lower));
        }

    }
    LPINFO((sumMinLog / rowCount) << ", " << (sumMaxLog / rowCount) << "  " << (sumDiffLog / rowCount));
    return upper - lower;
}
