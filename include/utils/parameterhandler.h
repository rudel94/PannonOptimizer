/**
 * @file parameterhandler.h
 */

#ifndef PARAMETERHANDLER_H
#define PARAMETERHANDLER_H

#include <map>
#include <vector>

#include "utils/parameter.h"

class ParameterHandler{
    friend class LinalgParameterHandlerTestSuite;
    friend class SimplexParameterHandlerTestSuite;

public:
    virtual ~ParameterHandler();

    double getParameterValue(const std::string& name);
    void setParameterValue(std::string name, double value);

    virtual void readParameterFile(const std::string filename);
    virtual void writeParameterFile() = 0;


protected:
    std::map<std::string, Parameter> m_values;

    virtual void initParameters() = 0;
    void loadValuesFromFile(std::ifstream& in);

    std::vector<std::string> tokenizer(std::string& line);
    std::string ignoreEmptyRows(std::ifstream& in);

};

#endif // PARAMETERHANDLER_H