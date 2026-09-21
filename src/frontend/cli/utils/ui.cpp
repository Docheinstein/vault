#include "utils/ui.h"

#include "utils/colors.h"

// Pretty print secret names with the form <site>/<account>
// by giving different colors to the two components.
SecureString get_secret_colorized_name(const SecureString& name) {

    const unsigned char* c = name.data();

    SecureString formatted_name {};
    formatted_name.append(BOLD);
    formatted_name.append(CYAN);

    bool separator_found = false;

    for (uint32_t i = 0; i < name.size(); ++i) {
        bool separator = !separator_found && *c == '/';

        if (separator) {
            formatted_name.append(RESET);
        }

        formatted_name.append(c, 1);

        if (separator) {
            formatted_name.append(YELLOW);
        }

        separator_found = separator_found || separator;

        ++c;
    }

    formatted_name.append(RESET);

    return formatted_name;
}
