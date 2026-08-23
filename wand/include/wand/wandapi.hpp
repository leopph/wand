#pragma once

#ifdef WAND_BUILD
#define WANDAPI __declspec(dllexport)
#else
#define WANDAPI __declspec(dllimport)
#endif
