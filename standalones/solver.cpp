#include <globals.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>

#include <lp/model.h>
#include <lp/mpsproblem.h>
#include <simplex/dualsimplex.h>
#include <simplex/primalsimplex.h>
#include <simplex/simplexparameterhandler.h>
#include <linalg/linalgparameterhandler.h>

enum ALGORITHM {
    PRIMAL,
    DUAL
};

void solve(std::string filename, ALGORITHM algorithm) {
    //Init
    Simplex* simplex;
    //TODO: Csinal maganak parameter fajlt, akkor is ha nem fut le
     if(algorithm == PRIMAL){
        simplex = new PrimalSimplex();
    } else if(algorithm == DUAL){
        simplex = new DualSimplex();
    }
    Model model;
    MpsModelBuilder builder;
    builder.loadFromFile(filename.c_str());
    model.build(builder);

    simplex->setModel(model);
    simplex->solve();

    //Release
    if(simplex != NULL){
        delete simplex;
        simplex = NULL;
    }
}

void printHelp() {
    std::cout << "Usage: NewPanOptDual [OPTION] [FILE] \n" <<
                 "Solve [FILE] with the dual simplex method. \n"<<
                 "\n"<<
                 "Algorithm specific parameters can be given in the .PAR parameter files. \n"<<
                 "If these files not exist, use the `-p` argument to generate them. \n"<<
                 "\n"<<
                 "  -a, --algorithm \t Specifies the solution algorithm (`primal` or `dual`(default)) .\n"<<
                 "  -d, --directory \t Solve every MPS file listed in the FILE directory.\n"<<
                 "  -f, --file      \t Solve an MPS file.\n"<<
                 "  -fl, --file-list \t Solve all the MPS files listed in text file.\n"<<
                 "  -p, --parameter-file \t Generate the default parameter files.\n"<<
                 "  -h, --help      \t Displays this help.\n"<<
                 "\n";
}

bool isFile(std::string path){
    struct stat s;
    if( stat(path.c_str(),&s) == 0 )
    {
        if( s.st_mode & S_IFREG )
        {
            return true;
        }
    }
    return false;
}

bool isDir(std::string path){
    struct stat s;
    if( stat(path.c_str(),&s) == 0 )
    {
        if( s.st_mode & S_IFDIR )
        {
            return true;
        }
    }
    return false;
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
        std::cout << "Default paramterer files generated! \n";
    } else {
        std::cout << "Error opening the working directory.\n";
    }
}

void solveDir(std::string dirPath, ALGORITHM algorithm) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (dirPath.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            std::string entry(ent->d_name);
            std::cout << "ENTRY: "<<entry << "\n";
            if(entry.size()>=4 && entry.substr(entry.size()-4 , 4).compare(".MPS") == 0){
                std::string filePath = dirPath;
                filePath.append({PATH_SEPARATOR}).append(entry);
                solve(filePath, algorithm);
            }
        }
        closedir (dir);
    } else {
        std::cout << "Error opening the working directory.\n";
    }
}

void solveFileList(std::string fileListPath, ALGORITHM algorithm) {
    std::string line;
    std::ifstream fileList(fileListPath);
    if(fileList.is_open()) {
        while(getline(fileList,line) ) {
            std::cout << "LINE: "<<line << "\n";
            if(line.size()>=4 && line.substr(line.size()-4 , 4).compare(".MPS") == 0){
                solve(line, algorithm);
            } else {
                std::cout << "Invalid record in the list: "<<line << "\n";
            }

        }
        fileList.close();
    }
    else {
        std::cout << "Unable to open file list.\n";
    }
}

int main(int argc, char** argv) {
    setbuf(stdout, 0);

    ALGORITHM algorithm = DUAL;

    if(argc < 2){
        printHelp();
    } else if(argc > 1){
        for(int i=1; i<argc; i++){
            std::string arg(argv[i]);
//            std::cout << "arg: "<< arg << "\n";
            if(arg.compare("-h") == 0 || arg.compare("--help") == 0){
                printHelp();
            } else if(arg.compare("-a") == 0 || arg.compare("--algorithm") == 0){
                if(argc < i+2 ){
                    printMissingOperandError(argv);
                    break;
                } else {
                    std::string alg(argv[i+1]);
                    if(alg.compare("primal") == 0){
                        algorithm = PRIMAL;
                    } else if(alg.compare("dual") == 0){
                        algorithm = DUAL;
                    } else {
                        std::cout << "Unknown algorithm, please use `primal` or `dual` (default).\n";
                    }
                    i++;
                }
            } else if(arg.compare("-d") == 0 || arg.compare("--directory") == 0){
                if(argc < i+2 ){
                    printMissingOperandError(argv);
                    break;
                } else {
                    std::string path(argv[i+1]);
                    if(isDir(path)){
                        solveDir(path, algorithm);
                        i++;
                    } else {
                        printInvalidOperandError(argv, i, i+1);
                        break;
                    }
                }
            } else if(arg.compare("-f") == 0 || arg.compare("--file") == 0){
                if(argc < i+2 ){
                    printMissingOperandError(argv);
                    break;
                } else {
                    std::string path(argv[i+1]);
                    if(isFile(path)){
                        solve(path, algorithm);
                        i++;
                    } else {
                        printInvalidOperandError(argv, i, i+1);
                        break;
                    }
                }
            } else if(arg.compare("-fl") == 0 || arg.compare("--file-list") == 0){
                if(argc < i+2 ){
                    printMissingOperandError(argv);
                    break;
                } else {
                    std::string path(argv[i+1]);
                    if(isFile(path)){
                        solveFileList(path, algorithm);
                        i++;
                    } else {
                        printInvalidOperandError(argv, i, i+1);
                        break;
                    }
                }
            } else if(arg.compare("-p") == 0 || arg.compare("--parameter-file") == 0){
                generateParameterFiles();
            } else {
                printInvalidOptionError(argv, i);
                break;
            }
        }
    }


    return EXIT_SUCCESS;
}
