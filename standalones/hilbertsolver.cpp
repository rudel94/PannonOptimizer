#include <globals.h>

#include <string>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>
#include <dirent.h>

#include <lp/model.h>
#include <lp/mpsproblem.h>
#include <lp/presolver.h>
#include <simplex/dualsimplex.h>
#include <simplex/primalsimplex.h>
#include <simplex/simplexparameterhandler.h>
#include <linalg/linalgparameterhandler.h>
#include <simplex/simplexcontroller.h>
#include <simplex/simplexsolver.h>
#include <lp/manualmodelbuilder.h>

#include <utils/tokenizer.h>
#include <simplex/checker.h>

void solve(unsigned int size) {
    Model model;
    ManualModelBuilder * builder = new ManualModelBuilder;

    unsigned int index;
    Numerical::Double * data = new Numerical::Double[size];
    unsigned int * indices = new unsigned int[size];
    for (index = 0; index < size; index++) {
        indices[index] = index;
    }

    Variable defaultVariable = Variable::createBoundedTypeVariable(nullptr, -1000, 1000);
    for (index = 0; index < size; index++) {

        int column;
        double sum = 0.0;
        for (column = size - 1; column >= 0; column--) {
            data[column] = 1.0 / (index + column + 1);
            sum += 1.0 / (index + column + 1);
        }
        ostringstream str;
        str << "ROW" << index;
        Constraint newConstraint = Constraint::createEqualityTypeConstraint(str.str().c_str(), sum);
        builder->addConstraint(newConstraint, data, indices, size, &defaultVariable);
    }

    for (index = 0; index < size; index++) {
        ostringstream str;
        str << "COL" << index;
        builder->setVariable(index, Variable::createBoundedTypeVariable(str.str().c_str(), -1000, 1000));
        builder->setCostCoefficient(index, 0.0);
    }

    model.build(*builder);
    delete builder;



    if(SimplexParameterHandler::getInstance().getBoolParameterValue("Starting.Presolve.enable") == true){
        Presolver presolver(&model);
        try {
            presolver.presolve();
            presolver.printStatistics();
            //            exit(-1);
        } catch(Presolver::PresolverException e) {
            LPERROR("[Presolver] " << e.getMessage());
            exit(-1);
        }
    }

    if(SimplexParameterHandler::getInstance().getBoolParameterValue("Starting.Scaling.enable") == true){
        model.scale();
    }

    //init simplexController
    //SimplexController simplexController;
    //simplexController.solve(model);
    SimplexSolver solver;
    solver.solve(model);
}

void printHelp() {
    std::cout << "Usage: NewPanOptDual [OPTION] [FILE] \n" <<
                 "Solve [FILE] with the dual simplex method. \n"<<
                 "\n"<<
                 "Algorithm specific parameters can be given in the .PAR parameter files. \n"<<
                 "If these files not exist, use the `-p` argument to generate them. \n"<<
                 "\n"<<
                 "   -$PARAMETER_NAME \t Specifies the value of a parameter .\n"<<
                 "   --hilbert <size> \t Solve a Hilbert matrix problem. \n" <<
                 "   -p, --parameter-file \t Generate the default parameter files.\n"<<
                 "   -o, --output    \t Redirect the solver output to a file.\n"<<
                 "   -h, --help      \t Displays this help.\n"<<
                 "\n";
}

void printInvalidOptionError(char** argv, int index) {
    std::cout << argv[0]<< ": invalid option: `" << argv[index]<<"`\n"<<
                           "Try `"<< argv[0] <<" --help` for more information.\n";
}

void printInvalidOperandError(char** argv, int opindex, int argindex) {
    std::cout << argv[0]<< ": invalid operand: `" << argv[argindex]<<"` for option: `"<< argv[opindex] <<"`\n"<<
                           "Try `"<< argv[0] <<" --help` for more information.\n";
}

void printMissingOperandError(char** argv) {
    std::cout << argv[0]<< ": missing operand \n"<<
                           "Try `"<< argv[0] <<" --help` for more information.\n";
}

void generateParameterFiles() {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (".")) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            std::string entry(ent->d_name);
            if(entry.size()>=4 && entry.substr(entry.size()-4 , 4).compare(".PAR") == 0){
                remove(entry.c_str());
                std::cout << "File " << entry << " removed! \n";
            }
        }
        closedir (dir);
        SimplexParameterHandler::getInstance().initParameters();
        SimplexParameterHandler::getInstance().writeParameterFile();
        LinalgParameterHandler::getInstance().initParameters();
        LinalgParameterHandler::getInstance().writeParameterFile();
        InitPanOpt::getInstance().getArchitecture().generateParameterFile();
        std::cout << "Default paramterer files generated! \n";
    } else {
        std::cout << "Error opening the working directory.\n";
    }
}

void redirectOutput(std::string path) {
    OutputHandler::getInstance().disableAllColors();
    freopen(path.c_str(), "a", stderr);
}

bool setParameter(ParameterHandler& handler, const std::string& arg, const char * value){
    if(handler.getParameterType(arg) == Entry::BOOL){
        if(strcmp(value,"true") == 0){
            handler.setParameterValue(arg, true);
        } else if(strcmp(value,"false") == 0){
            handler.setParameterValue(arg, false);
        } else {
            return false;
        }
    } else if(handler.getParameterType(arg) == Entry::DOUBLE) {
        handler.setParameterValue(arg, atof(value));
    } else if(handler.getParameterType(arg) == Entry::INTEGER) {
        handler.setParameterValue(arg, atoi(value));
    } else if(handler.getParameterType(arg) == Entry::STRING) {
        handler.setParameterValue(arg, std::string(value));
    } else {
        return false;
    }
    return true;
}

int main(int argc, char** argv) {

    setbuf(stdout, 0);

    bool outputRedirected = false;

    ParameterHandler& linalgHandler = LinalgParameterHandler::getInstance();
    ParameterHandler& simplexHandler = SimplexParameterHandler::getInstance();

    if(argc < 2){
        printHelp();
    } else if(argc > 1){
        for(int i=1; i<argc; i++){
            std::string arg(argv[i]);
            if (arg.compare("--hilbert") == 0) {
                if (argc < i + 2) {
                    printMissingOperandError(argv);
                    break;
                } else {
                    unsigned int size = atoi(argv[i+1]);
                    solve(size);
                    i++;
                }
            } else if(arg.compare("-h") == 0 || arg.compare("--help") == 0){
                printHelp();
            } else if(arg.compare("-p") == 0 || arg.compare("--parameter-file") == 0){
                generateParameterFiles();
            } else if(arg.compare("-o") == 0 || arg.compare("--output") == 0){
                if(argc < i+2 ){
                    printMissingOperandError(argv);
                    break;
                } else {
                    std::string path(argv[i+1]);
                    redirectOutput(path);
                    outputRedirected = true;
                    i++;
                }
            } else if(arg.compare(0,1,"-") == 0) {
                arg.erase(arg.begin());
                //Check if the given value is a linalg parameter
                if(linalgHandler.hasParameter(arg)){
                    if(!setParameter(linalgHandler,arg,argv[i+1])){
                        printInvalidOperandError(argv, i, i+1);
                        break;
                    }
                }
                //Check if the givel value is a simplex parameter
                else if(simplexHandler.hasParameter(arg)){
                    if(!setParameter(simplexHandler,arg,argv[i+1])){
                        printInvalidOperandError(argv, i, i+1);
                        break;
                    }
                } else {
                    printInvalidOptionError(argv, i);
                    break;
                }
                i++;
            } else {
                printInvalidOptionError(argv, i);
                break;
            }
        }
    }

    if(outputRedirected){
        fclose(stdout);
    }
    return EXIT_SUCCESS;
}

