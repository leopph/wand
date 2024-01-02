#pragma once

#ifdef WAND_EXPORT
#define WANDAPI __declspec(dllexport)
#else
#define WANDAPI __declspec(dllimport)
#endif
