#ifndef QUERY_COMMAND_H
#define QUERY_COMMAND_H

#include "utility/command_parser.h"
#include <map>
#include <iostream>

enum OptionKeyword {
    LabelFile = 1,     // -l, The label file path, compulsive parameter
    IQueryGraphFile = 2,      // -iq, The query graph file path, compulsive parameter
    OQueryGraphFile = 3      // -oq, The query graph file path, compulsive parameter
};

class QueryCommand : public CommandParser{
private:
	//the pair <actual meaning, command abbreviation>
    std::map<OptionKeyword, std::string> options_key;
    //the pair <actual meaning, command parameter>
    std::map<OptionKeyword, std::string> options_value;

private:
    void processOptions();

public:
    QueryCommand(int argc, char **argv);

    std::string getIQueryGraphFile() {
        return options_value[OptionKeyword::IQueryGraphFile];
    }

    std::string getOQueryGraphFile() {
        return options_value[OptionKeyword::OQueryGraphFile];
    }

    std::string getLabelFilePath() {
        return options_value[OptionKeyword::LabelFile];
    }
};

#endif