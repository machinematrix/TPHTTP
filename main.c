#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "HttpServer.h"
#include "Vector.h"

const char *programName;
VectorHandle files;

void logger(const char *msg)
{
	printf("%s\n", msg);
}

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
		*outSize = 0;
		*outBuffer = NULL;
	}
}

VectorHandle loadDirectoryFileNames(const char *directory, const char *extension)
{
	VectorHandle files = Vector_Create(sizeof(char*));
	DIR *currDir = opendir(directory);

	if (files && currDir)
	{
		struct dirent *entry;

		while (entry = readdir(currDir))
		{
			if (!extension || (strlen(extension) < strlen(entry->d_name) && !strcmp(entry->d_name + strlen(entry->d_name) - strlen(extension), extension)))
			{
				char *auxName = malloc(strlen(entry->d_name) + 1);

				if (auxName)
				{
					strcpy(auxName, entry->d_name);
					Vector_PushBack(files, &auxName);
				}
			}
		}
	}

	return files;
}

void freeDirectoryFileNames(VectorHandle files)
{
	if (files)
	{
		size_t i, sz;
		for (i = 0, sz = Vector_Size(files); i < sz; ++i)
			free(*(char**)Vector_At(files, i));

		Vector_Destroy(files);
	}
}

void list(HttpRequestHandle req)
{
	HttpResponseHandle response = HttpServer_CreateResponse(req);
	char strBodySize[16];
	const char *bodyBeg = "<a href=\"/", *bodyMid = "\">", *bodyEnd = "</a><br />";
	char *body;
	size_t bodySize = 0, i, sz, bodyBegSize = strlen(bodyBeg), bodyMidSize = strlen(bodyMid), bodyEndSize = strlen(bodyEnd);
	int offset;

	for (i = 0, sz = Vector_Size(files); i < sz; ++i)
	{
		bodySize += bodyBegSize + strlen(*(char**)Vector_At(files, i)) + bodyMidSize + strlen(*(char**)Vector_At(files, i)) + bodyEndSize;
	}

	body = malloc(bodySize);
	sprintf(strBodySize, "%d", bodySize);

	for (i = 0, sz = Vector_Size(files), offset = 0; i < sz; ++i)
	{
		offset += sprintf(body + offset, "%s%s%s%s%s", bodyBeg, *(char**)Vector_At(files, i), bodyMid, *(char**)Vector_At(files, i), bodyEnd);
	}

	HttpServer_SetResponseStatusCode(response, 200);
	HttpServer_SetResponseField(response, HttpResponseField_ContentType, "text/html");
	HttpServer_SetResponseField(response, HttpResponseField_ContentLength, strBodySize);
	HttpServer_SetResponseField(response, HttpResponseField_Connection, "close");
	HttpServer_SetResponseBody(response, body, bodySize);
	HttpServer_SendResponse(response);

	free(body);

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

void redirectToList(HttpRequestHandle req)
{
	HttpResponseHandle resp = HttpServer_CreateResponse(req);

	if (resp)
	{
		HttpServer_SetResponseStatusCode(resp, 303);
		HttpServer_SetResponseField(resp, HttpResponseField_ContentType, "text/html");
		HttpServer_SetResponseField(resp, HttpResponseField_Location, "/list");
		HttpServer_SendResponse(resp);
	}

	HttpServer_DestroyResponse(resp);
}

int main(int argc, char **argv)
{
	ServerError error;
	programName = basename(argv[0]);
	HttpServerHandle server = HttpServer_Create(80, &error);

	if (server)
	{
		files = loadDirectoryFileNames(".", ".jpg");

		if (files)
		{
			size_t i, sz;
			for (i = 0, sz = Vector_Size(files); i < sz; ++i)
			{
				char *resourcePath = malloc(strlen(*(char**)Vector_At(files, i)) + 2);
				if (resourcePath)
				{
					sprintf(resourcePath, "/%s", *(char**)Vector_At(files, i));
					HttpServer_SetEndpointCallback(server, resourcePath, image);
					free(resourcePath);
				}
			}


			HttpServer_SetEndpointCallback(server, "/list", list);
			HttpServer_SetEndpointCallback(server, "/", redirectToList);
			HttpServer_SetLoggerCallback(server, logger);

			HttpServer_Start(server);

			if (HttpServer_GetStatus(server) == ServerStatus_Running)
			{
				char continuar[50] = { 0 };

				while (strcmp(continuar, "exit"))
				{
					scanf("%s", &continuar);
				}
			}
			else
				printf("%s\n", HttpServer_GetServerError(HttpServer_GetErrorCode(server)));
		}

		freeDirectoryFileNames(files);
		HttpServer_Destroy(server);
	}
	else printf("Could not create server\n");

    return 0;
}