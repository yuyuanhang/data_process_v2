#include "csv_command.h"

CSVCommand::CSVCommand(const int argc, char **argv) : CommandParser(argc, argv) {
    // Initialize options value
    options_key[OptionKeyword::CSVFile] = "-i";
    options_key[OptionKeyword::GraphFile] = "-g";
    options_key[OptionKeyword::LabelFile] = "-l";
    processOptions();
};

void CSVCommand::processOptions() {
    // CSV file path
    options_value[OptionKeyword::CSVFile] = getCommandOption(options_key[OptionKeyword::CSVFile]);;

    // Data graph file path
    options_value[OptionKeyword::GraphFile] = getCommandOption(options_key[OptionKeyword::GraphFile]);

    // Label file path
    options_value[OptionKeyword::LabelFile] = getCommandOption(options_key[OptionKeyword::LabelFile]);
}