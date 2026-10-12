#include "scripts_extract.hpp"

#include <iostream>

int main(void)
{
	try
	{
		const size_t count = scripts_extract("Scripts.rvdata2", "Scripts");

		std::cout << "Extracted " << count << " scripts into Scripts/ and wrote Scripts/scripts.tree" << std::endl;
		return 0;
	}
	catch (const std::exception &ex)
	{
		std::cerr << "error: " << ex.what() << std::endl;
		return 1;
	}
}
