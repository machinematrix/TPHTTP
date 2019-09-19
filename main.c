#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "HttpServer.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

const char *programName;

void loadFile(FILE *file, void **outBuffer, long *outSize)
{
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);
	void *buffer = malloc((size_t)size);
	
	if (buffer) {
		fread(buffer, 1, (size_t)size, file);
		*outSize = size;
		*outBuffer = buffer;
	}
	else {
		*outBuffer = NULL;
		*outSize = 0;
	}
}

void loadDirectoryFileNames(const char *directory, char ***outFileNames, size_t *outFileCount)
{
	DIR *currDir = opendir(directory);
	*outFileNames = NULL;
	*outFileCount = 0;

	if (currDir)
	{
		size_t count = 0, capacity = 2;
		*outFileNames = malloc(capacity * sizeof(char*));

		if (*outFileNames)
		{
			struct dirent *entry;
			while ((entry = readdir(currDir)))
			{
				char *auxName = malloc(strlen(entry->d_name) + 1);
				
				if (auxName)
				{
					if (count == capacity) {
						capacity *= 2;
						*outFileNames = realloc(*outFileNames, capacity * sizeof(char *));
					}
					strcpy(auxName, entry->d_name);
					(*outFileNames)[count++] = auxName;
				}
			}
			if (count)
				*outFileCount = count;
		}
	}
}

void freeDirectoryFileNames(char **fileNames, size_t fileCount)
{
	if (fileNames) {
		size_t i;
		for (i = 0; i < fileCount; ++i)
			free(fileNames[i]);
		free(fileNames);
	}
}

void list(HttpRequestHandle req)
{
	HttpResponseHandle response = HttpServer_CreateResponse(req);
	char **names, strBodySize[16];
	const char *bodyBeg = "<a href=\"/", *bodyMid = "\">", *bodyEnd = "</a><br />";
	char *body;
	size_t count = 2, bodySize = 0, i, bodyBegSize = strlen(bodyBeg), bodyMidSize = strlen(bodyMid), bodyEndSize = strlen(bodyEnd);
	int offset;

	loadDirectoryFileNames(".", &names, &count);
	
	if (names)
	{
		for (i = 0; i < count; ++i) {
			if (strcmp(names[i], "..") && strcmp(names[i], ".") && strcmp(names[i], programName))
				bodySize += bodyBegSize + strlen(names[i]) + bodyMidSize + strlen(names[i]) + bodyEndSize;
		}

		body = malloc(bodySize);
		sprintf(strBodySize, "%d", bodySize);

		for (i = 0, offset = 0; i < count; ++i) {
			if (strcmp(names[i], "..") && strcmp(names[i], ".") && strcmp(names[i], programName))
				offset += sprintf(body + offset, "%s%s%s%s%s", bodyBeg, names[i], bodyMid, names[i], bodyEnd);
		}
		freeDirectoryFileNames(names, count);

		HttpServer_SetResponseStatusCode(response, 200);
		HttpServer_SetResponseField(response, HttpResponseField_ContentType, "text/html");
		HttpServer_SetResponseField(response, HttpResponseField_ContentLength, strBodySize);
		HttpServer_SetResponseField(response, HttpResponseField_Connection, "close");
		HttpServer_SetResponseBody(response, body, bodySize);
		HttpServer_SendResponse(response);

		free(body);
	}

	HttpServer_DestroyResponse(response);
}

void image(HttpRequestHandle req)
{
	HttpResponseHandle response = HttpServer_CreateResponse(req);
	FILE *file = fopen(HttpServer_GetRequestUri(req) + 1, "rb");

	if (file)
	{
		void *fileBuffer = NULL;
		long fileSize = 0;
		loadFile(file, &fileBuffer, &fileSize);

		if (fileBuffer) {
			char strSize[32];
			sprintf(strSize, "%d", fileSize);
			HttpServer_SetResponseStatusCode(response, 200);
			HttpServer_SetResponseField(response, HttpResponseField_ContentLength, strSize);
			HttpServer_SetResponseField(response, HttpResponseField_ContentType, "image/jpeg");
			HttpServer_SetResponseField(response, HttpResponseField_Connection, "close");
			HttpServer_SetResponseBody(response, fileBuffer, (size_t)fileSize);
			
			free(fileBuffer);
		}
		else {
			HttpServer_SetResponseStatusCode(response, 404);
		}

		fclose(file);
	}
	else {
		HttpServer_SetResponseStatusCode(response, 404);
	}

	HttpServer_SendResponse(response);
	HttpServer_DestroyResponse(response);
}

int main(int argc, char **argv)
{
	programName = basename(argv[0]);
    HttpServerHandle sv = HttpServer_Create(80);

	if (sv)
	{
		char **names;
		size_t count;
		loadDirectoryFileNames(".", &names, &count);
		if (names)
		{
			size_t i;
			for (i = 0; i < count; ++i)
			{
				if (strcmp(names[i], ".") && strcmp(names[i], "..") && strcmp(names[i], basename(argv[0])))
				{
					char *resourcePath = malloc(strlen(names[i]) + 2);
					resourcePath[0] = '/';
					strcpy(resourcePath + 1, names[i]);
					HttpServer_SetEndpointCallback(sv, resourcePath, image);
					free(resourcePath);
				}
			}
		}
		freeDirectoryFileNames(names, count);
		HttpServer_SetEndpointCallback(sv, "/list", list);

		HttpServer_Start(sv);

		if (HttpServer_GetStatus(sv) == Running)
		{
			int continuar = 1;

			while (continuar)
			{
				scanf("%d", &continuar);
			}
		}
		else
			printf("%s\n", HttpServer_GetServerError(sv));

		HttpServer_Destroy(sv);
	}
	else printf("Could not create server\n");

    return 0;
}