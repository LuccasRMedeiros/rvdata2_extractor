#ifndef __SCRIPTS_EXTRACT_HPP__
#define __SCRIPTS_EXTRACT_HPP__

#include <cstddef>
#include <string>

/* Reads Scripts.rvdata2 and writes numbered .rb files into output_dir.
 * Returns the number of scripts written.
 */
size_t scripts_extract(const std::string &input_path, const std::string &output_dir);

#endif // __SCRIPTS_EXTRACT_HPP__
