#include <direct.h>
#include <string.h>

void
foreach_pcx(void (*fn)(char *filename))
{
	struct dirent *de;
	DIR *d;

	d = opendir(".");
	if (d == NULL)
		return;

	while ((de = readdir(d)) != NULL) {
		char *ext;

		ext = strrchr(de->d_name, '.');
		if (ext == NULL)
			continue;

		if (strcmp(ext, ".PCX") == 0)
			fn(de->d_name);
	}

	closedir(d);
}

int
pcx_present(void)
{
	struct dirent *de;
	DIR *d;
	int r;

	d = opendir(".");
	if (d == NULL)
		return 0;

	r = 0;
	while ((de = readdir(d)) != NULL) {
		char *ext;

		ext = strrchr(de->d_name, '.');
		if (ext == NULL)
			continue;

		if (strcmp(ext, ".PCX") == 0) {
			r = 1;
			break;
		}
	}

	closedir(d);
	return r;
}
