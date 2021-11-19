#include "query_command.h"

QueryCommand::QueryCommand(const int argc, char **argv) : CommandParser(argc, argv) {
    // Initialize options value
    options_key[OptionKeyword::IQueryGraphFile] = "-iq";
    options_key[OptionKeyword::OQueryGraphFile] = "-oq";
    options_key[OptionKeyword::LabelFile] = "-l";
    processOptions();
};

void QueryCommand::processOptions() {
    // Input query graph file path
    options_value[OptionKeyword::IQueryGraphFile] = getCommandOption(options_key[OptionKeyword::IQueryGraphFile]);

    // Output query graph file path
    options_value[OptionKeyword::OQueryGraphFile] = getCommandOption(options_key[OptionKeyword::OQueryGraphFile]);

    // Label file path
    options_value[OptionKeyword::LabelFile] = getCommandOption(options_key[OptionKeyword::LabelFile]);
}