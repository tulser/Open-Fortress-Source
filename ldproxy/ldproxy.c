#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

static void load_lib(void) __attribute__((constructor));
static void unload_lib(void) __attribute__((destructor));

typedef void *(*CreateInterfaceFn)(const char *pName, int *pReturnCode);

static void *g_client;
static CreateInterfaceFn g_CreateInterface;

static void load_lib(void)
{
    printf("LDPROXY LOADING..\n");

    // Step 1. Parse /proc/{}/maps for our path
    char *our_path=0;
    int our_path_len;
    char line[512];
    {
        FILE *procmaps = fopen("/proc/self/maps", "r");
        while (fgets(line, sizeof(line), procmaps))
        {
            int start, end;
            sscanf(line, "%*x-%*x %*4c %*x %*x:%*x %*x %n", &start);
            char *path = line+start-1;
            end = strlen(path);
            //sizeof("ldproxy/client.so\n")==18
            int clen = 18;
            if(end>=clen&&!strcmp(&path[end-clen], "ldproxy/client.so\n")){
                //clip off the ldproxy/client.so\n
                path[end-clen]='\0';
                our_path = path;
                our_path_len = end-clen;
                while(*our_path==' '){
                    our_path++;
                    our_path_len--;
                }
                break;
            }
        }
    }
    assert(our_path);
    // Step 2. Modify LD_LIBRARY_PATH
    char *path = getenv("LD_LIBRARY_PATH");
    char *newpath;
    if(path){
        int newlen = strlen(path) + 1 + our_path_len;
        newpath = malloc(newlen);
        snprintf(newpath, newlen, "%s:%s", our_path, path);
    }else{
        newpath = our_path;
    }
    setenv("LD_LIBRARY_PATH", newpath, true);
    if(path) free(newpath);
    // Step 3. Load real client.so
    strcat(our_path, "client.so");
    assert(g_client = dlopen(our_path, RTLD_NOW));
    assert(g_CreateInterface = dlsym(g_client, "CreateInterface"));
    printf("LDPROXY SUCCESS\n");
}

static void unload_lib(void)
{
    dlclose(g_client);
}

__attribute__((visibility("default")))
void *CreateInterface(const char *pName, int *pReturnCode){
   return g_CreateInterface(pName, pReturnCode);
}
