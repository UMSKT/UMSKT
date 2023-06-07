//
// Created by Andrew on 01/06/2023.
//

#include "header.h"
#include "cli.h"

Options options;

int main(int argc, char *argv[]) {
    if (!CLI::parseCommandLine(argc, argv, &options)) {
        fmt::print("error parsing command line options\n");
        CLI::showHelp(argv);
        return !options.error ? 0 : 1;
    }

    json keys;

    int status = CLI::validateCommandLine(&options, argv, &keys);
    if (status > 0) {
        return status;
    }

    CLI* run = new CLI(options, keys);

    switch(options.applicationMode) {
        case MODE_BINK1998:
            return run->BINK1998();

        case MODE_BINK2002:
            return run->BINK2002();

        case MODE_CONFIRMATION_ID:
            return run->ConfirmationID();

        default:
            return 1;
    }
}
