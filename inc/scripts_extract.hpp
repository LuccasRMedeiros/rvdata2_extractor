#ifndef __SCRIPTS_EXTRACT_HPP__
#define __SCRIPTS_EXTRACT_HPP__

#include <cstddef>
#include <string>

/* Reads Scripts.rvdata2 and writes .rb files plus scripts.tree into output_dir.
 * Mounts an existing scripts.tree when present so known ids keep their paths.
 * Titles containing ▼ become directories under output_dir.
 * Returns the number of script files written.
 */
size_t scripts_extract(const std::string &input_path, const std::string &output_dir);

#endif // __SCRIPTS_EXTRACT_HPP__
