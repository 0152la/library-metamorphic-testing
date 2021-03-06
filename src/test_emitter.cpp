#include "test_emitter.hpp"

static unsigned int indent = 0;
const std::string default_config_file =
    "/home/sentenced/Documents/Internships/2018_ETH/work/sets/config_files/config_isl.yaml";

//#if DEBUG
//bool DEBUG = true;
//#else
bool DEBUG = false;
//#endif
bool META_TESTING = true;

std::map<std::string, Modes> string_to_mode {
    {"SET_FUZZ", SET_FUZZ},
    {"API_FUZZ", API_FUZZ},
    {"SET_TEST", SET_TEST},
    {"SET_META_STR", SET_META_STR},
    {"SET_META_API", SET_META_API},
    {"SET_META_NEW", SET_META_NEW},
};

void
parseArgs(Arguments& args, int argc, char **argv)
{
    int i = 1;
    while (i < argc)
    {
        /* Arguments with options required */
        if (!strcmp(argv[i], "--seed") || !strcmp(argv[i], "-s"))
        {
            args.seed = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--config-file") || !strcmp(argv[i], "-c"))
        {
            args.config_file = argv[++i];
        }
        else if (!strcmp(argv[i], "--output-file") || !strcmp(argv[i], "-o"))
        {
            args.output_file = argv[++i];
        }
        /* Flag arguments */
        else if (!strcmp(argv[i], "--debug"))
        {
            DEBUG = true;
        }
        else {
            std::cout << "Found unknown argument: " << argv[i] << std::endl;
            exit(1);
        }
        i++;
    }
}

YAML::Node
loadYAMLFileWithCheck(const std::string& file_path)
{
    try
    {
        return YAML::LoadFile(file_path);
    }
    catch (const YAML::BadFile& e)
    {
        std::cout << "Failed loading YAML file at path " << file_path << std::endl;
        exit(1);
    }
}

void
writeLine(std::stringstream &ss, std::string line)
{
    int indent_count = 0;
    while (indent_count++ < indent)
        ss << "\t";
    ss << line << std::endl;
}

void
prepareHeader(std::stringstream &ss, std::vector<std::string> &include_list,
    Arguments& args, std::string api_file, std::string meta_file)
{
    writeLine(ss, fmt::format("// SEED : {}", args.seed));
    writeLine(ss, fmt::format("// API CONFIG FILE : {}", api_file));
    writeLine(ss, fmt::format("// META CONFIG FILE : {}", meta_file));
    std::time_t curr_time_t = std::time(nullptr);
    writeLine(ss, fmt::format("// GENERATION TIME : {}",
        std::ctime(&curr_time_t)));
    writeLine(ss, "");
    for (std::string incl : include_list)
        writeLine(ss, "#include " + incl);
}

void
mainPreSetup(std::stringstream &ss, std::vector<std::string>& pre_setup_instrs)
{
    writeLine(ss, "int main()");
    writeLine(ss, "{");
    indent++;
    for (std::string pre_setup_instr : pre_setup_instrs)
    {
        writeLine(ss, pre_setup_instr);
    }
}

void
mainPostSetup(std::stringstream &ss)
{
    indent--;
    writeLine(ss, "}");
}

int
main(int argc, char** argv)
{
    Arguments args;
    parseArgs(args, argc, argv);

    if (args.config_file.empty())
    {
        args.config_file = default_config_file;
    }

    YAML::Node config_data = loadYAMLFileWithCheck(args.config_file);
    std::string working_dir = config_data["working_dir"].as<std::string>();
    std::string api_fuzzer_path =
        working_dir + config_data["api_fuzzer_file"].as<std::string>();
    std::string meta_test_path =
        working_dir + config_data["meta_test_file"].as<std::string>();
    if (args.output_file.empty())
    {
        args.output_file =
            working_dir + config_data["test_emitter_output_file"]
                .as<std::string>();
    }

    std::mt19937* rng = new std::mt19937(args.seed);
    std::stringstream test_ss;

    YAML::Node api_fuzzer_data = loadYAMLFileWithCheck(api_fuzzer_path);
    std::vector<std::string> include_list = {
        "<cassert>",
        "<iostream>",
    };
    if (api_fuzzer_data["includes"].IsDefined())
    {
        for (YAML::Node include_yaml : api_fuzzer_data["includes"])
        {
            include_list.push_back(fmt::format("\"{}\"",
                include_yaml.as<std::string>()));
        }
    }

    prepareHeader(test_ss, include_list, args, api_fuzzer_path, meta_test_path);
    std::vector<std::string> pre_setup_instrs;
    if (api_fuzzer_data["pre_setup"].IsDefined())
    {
        for (YAML::Node pre_setup_yaml : api_fuzzer_data["pre_setup"])
        {
            pre_setup_instrs.push_back(pre_setup_yaml.as<std::string>());
        }
    }
    mainPreSetup(test_ss, pre_setup_instrs);

    std::unique_ptr<ApiFuzzerNew> api_fuzzer (
        new ApiFuzzerNew(api_fuzzer_path, meta_test_path, args.seed, rng));
    for (std::string instr : api_fuzzer->getInstrStrs())
    {
        writeLine(test_ss, instr);
    }

    mainPostSetup(test_ss);

    std::ofstream ofs;
    ofs.open(args.output_file);
    ofs << test_ss.rdbuf();
    ofs.close();
}
