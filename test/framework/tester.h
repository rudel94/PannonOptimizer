/* 
 * File:   tester.h
 * Author: smidla
 *
 * Created on 2013. march 2., 21:36
 */

#ifndef TESTER_H
#define	TESTER_H

#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <ctime>
#include <framework/report.h>

#include <utils/thirdparty/prettyprint.h>

struct TestResult
{
    std::string m_file;
    unsigned int m_line;
    bool m_result;
    std::string m_source;
};

class UnitTest;

class Tester
{
    friend class UnitTest;
public:

    static void addUnitTest(UnitTest * newTest)
    {
        sm_tests.push_back(newTest);
    }

    static void run();

    static void generateHtmlOutput(const char * directory);

    static void generateLatexOutput(const char * directory);

    static void setReportTitle(const char * title);

    static unsigned long long int getUnimplementedUnitTests();

    static unsigned long long int getTestFunctions();

    static unsigned long long int getFailedTestFunctions();

    static unsigned long long int getUnimplementedTestFunctions();

    static unsigned long long int getAsserts();

    static unsigned long long int getFailedAsserts();

    static std::string getStatusString();
private:

    static const unsigned int sm_simpleCell = 0;
    static const unsigned int sm_headCell = 1;
    static const unsigned int sm_sortCell = 2;

    struct ReportCell {
        std::string m_value;
        std::string m_attributes;
        unsigned int m_httpSettings;
    };

    static std::list<UnitTest*> sm_tests;
    static unsigned int sm_totalCounter;
    static unsigned int sm_subCounter;
    static bool sm_firstTest;
    static bool sm_actualIsGood;
    static unsigned int sm_totalErrorCounter;
    static std::string sm_extraInfo;
    static std::string sm_title;
    static tm sm_startTime;

    static double sm_executionTime;
    static unsigned int sm_failedUnitTests;


    // key: name of the test unit
    static std::map<std::string, std::map< std::string, std::list<TestResult> > > sm_results;

    static void addNewResult(std::string unit, std::string test, TestResult result)
    {
        std::map<std::string, std::map< std::string, std::list<TestResult> > >::iterator iter = sm_results.find(unit);
        std::map<std::string, std::map< std::string, std::list<TestResult> > >::iterator iterEnd = sm_results.end();
        if (iter == iterEnd) {
            /*if (!sm_firstTest) {
                if (sm_actualIsGood) {
                    std::cout << " PASSED" << std::endl;
                } else {
                    sm_totalErrorCounter++;
                    std::cout << " FAILED" << std::endl;
                }
            }*/
            sm_totalCounter++;
            sm_firstTest = false;
            sm_actualIsGood = true;
            sm_subCounter = 1;
            //std::cout << "\t" << sm_subCounter << ": " << test;

        } else {
            std::map< std::string, std::list<TestResult> >::iterator iter2 = (*iter).second.find(test);
            std::map< std::string, std::list<TestResult> >::iterator iterEnd2 = (*iter).second.end();
            if (iter2 == iterEnd2) { // new test case
                /*if (sm_actualIsGood) {
                    std::cout << " PASSED" << std::endl;
                } else {
                    sm_totalErrorCounter++;
                    std::cout << " FAILED" << std::endl;
                }*/

                sm_totalCounter++;
                sm_subCounter++;
                sm_actualIsGood = true;
                //std::cout << "\t" << sm_subCounter << ": " << test;
            }
        }
        if (result.m_result == false) {
            std::cout << std::endl << std::endl << "\tFile: " << result.m_file << std::endl;
            std::cout << "\tFunction: " << test << std::endl;
            std::cout << "\tLine: " << result.m_line << std::endl;
            std::cout << "\t" << result.m_source << std::endl;
            if (sm_extraInfo.length() > 0) {
                std::cout << "\t Extra info: " << sm_extraInfo << std::endl;
            }
            std::cout << std::endl;
            sm_actualIsGood = false;
        }
        sm_results[unit][test].push_back(result);
    }

    static void setExtraInfo(const std::string & info)
    {
        sm_extraInfo = info;
    }

    static void generateSummaryReport(ReportModule * module);

    static void generateHtmlHead(std::ostream & os);

    static void generateHtmlBody(std::ostream & os);

    static void generateHtmlSummary(std::ostream & os);

    static void generateUnitTestSummary(std::ostream & os, std::string *script);

    static void generateTable(std::ostream & os,
                              const std::vector<std::vector<ReportCell>> & table,
                              const std::string & name = "");

    static void generateTableScript(const std::vector<std::vector<ReportCell>> & table,
                                    const std::string &tableName,
                                    std::string * script);

    static std::string getCamelCase(const std::string & value, bool lower = false);
};

#endif	/* TESTER_H */

