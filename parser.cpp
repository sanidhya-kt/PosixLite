#include "shell.h"

std::vector<Command> parse(const std::vector<std::string>& tokens) {
    std::vector<Command> pipeline;
    Command cur;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& t = tokens[i];

        if (t == "|") {
            pipeline.push_back(cur);
            cur = Command{};
        } else if (t == "<") {
            if (i + 1 < tokens.size()) cur.input_file  = tokens[++i];
        } else if (t == ">>") {
            if (i + 1 < tokens.size()) { cur.output_file = tokens[++i]; cur.append_output = true; }
        } else if (t == ">") {
            if (i + 1 < tokens.size()) cur.output_file = tokens[++i];
        } else if (t == "&") {
            cur.background = true;
        } else {
            cur.args.push_back(t);
        }
    }
    pipeline.push_back(cur);
    return pipeline;
}
