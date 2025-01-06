#include <dir.h>

void
foreach_bmp(void (*fn)(char *fname))
{
	struct ffblk ffblk;
	int r;

	for (
		r = findfirst("*.bmp", &ffblk, 0);
		r == 0;
		r = findnext(&ffblk)
	) {
		fn(ffblk.ff_name);
	}
}

int
bmp_present(void)
{
	struct ffblk ffblk;

	return findfirst("*.bmp", &ffblk, 0) == 0;
}
