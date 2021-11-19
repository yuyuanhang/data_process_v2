#ifndef CSV_COMMAND_H
#define CSV_COMMAND_H

#include "utility/command_parser.h"
#include <map>
#include <iostream>

enum OptionKeyword {
    CSVFile = 1,     // -i, The csv file path, compulsive parameter
    GraphFile = 2,      // -g, The data graph file path, compulsive parameter
    LabelFile = 3      // -l, The label file path, compulsive parameter
};

class CSVCommand : public CommandParser{
private:
	//the pair <actual meaning, command abbreviation>
    std::map<OptionKeyword, std::string> options_key;
    //the pair <actual meaning, command parameter>
    std::map<OptionKeyword, std::string> options_value;

private:
    void processOptions();

public:
    CSVCommand(int argc, char **argv);

    std::string getCSVFilePath() {
        return options_value[OptionKeyword::CSVFile];
    }

    std::string getGraphFilePath() {
        return options_value[OptionKeyword::GraphFile];
    }

    std::string getLabelFilePath() {
        return options_value[OptionKeyword::LabelFile];
    }
};

#endif