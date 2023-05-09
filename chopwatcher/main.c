/* -*- c-basic-offset: 4; compile-command: "xcodebuild"; -*- */
//
//  main.c
//  chopwatcher
//
//  Created by Patrick Thomson on 4/21/23.
//

#include <stdio.h>
#include <dirent.h>
#include <libgen.h>
#include <CoreServices/CoreServices.h>

#define countof(x) (sizeof(x) / sizeof(x[0]))

static bool verbose = true;

void die(const char* message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

struct FileEvent {
    size_t index;
    char *path;
    FSEventStreamEventFlags flags;
    FSEventStreamEventId ident;
};

struct Action {
    char *name;
    FSEventStreamEventFlags flags;
};

#define _FLAG(name) kFSEventStreamEventFlag##name

const struct Action ALL_ACTIONS[] = {
    {
        .name = "Finder copy",
        .flags = _FLAG(ItemChangeOwner) | _FLAG(ItemCreated) | _FLAG(ItemInodeMetaMod) | _FLAG(ItemIsFile) | _FLAG(ItemCloned),
    },
    {
        .name = "Finder move",
        .flags = _FLAG(ItemIsFile) | _FLAG(ItemRenamed),
    },
    {
        .name = "NeuralMix create",
        .flags = _FLAG(ItemCreated) | _FLAG(ItemInodeMetaMod) | _FLAG(ItemIsFile) | _FLAG(ItemModified) | _FLAG(ItemXattrMod),
    },
};

void describe_event(struct FileEvent *evt) {
    printf("Event id %" PRIu64 ":\n", (uint64_t)evt->ident);
    printf("\tpath: %s\n", evt->path);
    printf("\tindex: %zu\n", evt->index);
#define _STR(s) #s
#define _GO(name) if (evt->flags & _FLAG(name)) printf("\t%s\n", _STR(name));
    _GO(MustScanSubDirs);
    _GO(UserDropped);
    _GO(KernelDropped);
    _GO(EventIdsWrapped);
    _GO(HistoryDone);
    _GO(RootChanged);
    _GO(Mount);
    _GO(Unmount);
    _GO(ItemChangeOwner);
    _GO(ItemCreated);
    _GO(ItemFinderInfoMod);
    _GO(ItemInodeMetaMod);
    _GO(ItemIsDir);
    _GO(ItemIsFile);
    _GO(ItemIsHardlink);
    _GO(ItemIsLastHardlink);
    _GO(ItemIsSymlink);
    _GO(ItemModified);
    _GO(ItemRemoved);
    _GO(ItemRenamed);
    _GO(ItemXattrMod);
    _GO(OwnEvent);
    _GO(ItemCloned);
    printf("\n");
#undef _GO
#undef _STR
}

#undef _FLAG

void onFileAdd(FSEventStreamRef ref,
               void *unused,
               size_t numEvents,
               char **eventPaths,
               const FSEventStreamEventFlags *eventFlags,
               const FSEventStreamEventId *eventIds)
{
    if (verbose) printf("Event count %zu\n", numEvents);
    for (size_t ii=0; ii<numEvents; ii++) {
        struct FileEvent evt =
            { .index = ii,
              .path = eventPaths[ii],
              .flags = eventFlags[ii],
              .ident = eventIds[ii],
            };
        if (verbose) describe_event(&evt);

        bool recognized = false;
        for (size_t action=0; action < countof(ALL_ACTIONS); action++) {
            if (evt.flags == ALL_ACTIONS[action].flags) {
                if (verbose) printf("Recognized %s\n", ALL_ACTIONS[action].name);
                recognized = true;
                break;
            }
        }
        if (!recognized) {
            printf("Ignoring...\n");
            continue;
        }

        char *base = basename(evt.path);
        char *filename = strdup(base);
        char *toFree = filename;
        char *foldername = strsep(&filename, "_");

        assert(filename != NULL && foldername != NULL);

        printf("Folder: %s, file: %s\n", foldername, filename);

        char *newpath = NULL;
        size_t nthfile = 1;
        asprintf(&newpath, "/Users/patrickt/beats/packs/%s", foldername);
        DIR *dir = opendir(newpath);
        if (dir == NULL) {
            char *err = strcat(newpath, strerror(errno));
            die(err);
        }

        struct dirent* entry;
        while ((entry = readdir(dir))) {
            char *extension = strrchr(entry->d_name, (int)'.');
            if (strcmp(extension, ".wav") == 0) nthfile++;
        }
        closedir(dir);
        nthfile--; // zero-indexed
        free(newpath);
        newpath = NULL;

        asprintf(&newpath, "/Users/patrickt/beats/packs/%s/%03zu_%s", foldername, nthfile, filename);
        assert(newpath != NULL);
        printf("New path: %s\n", newpath);
        if (rename(evt.path, newpath) != 0) {
            perror("Rename failed");
            fprintf(stderr, "(tried moving %s to %s)\n", evt.path, newpath);
            exit(EXIT_FAILURE);
        } else if (verbose) {
            fprintf(stderr, "mv %s %s\n", evt.path, newpath);
        }

        free(newpath);
        free(toFree);
    }
    if (verbose) printf("Done.\n");
}

int main(int argc, const char * argv[])
{
    if (argc != 2) die("Error: Directory not provided.");
    DIR *dir = opendir(argv[1]);
    if (dir == NULL) {
        die(strerror(errno));
    }

    dispatch_queue_t queue = dispatch_queue_create("chopwatcher", DISPATCH_QUEUE_SERIAL);
    dispatch_async(dispatch_get_main_queue(), ^{
            printf("Booting up…\n");
            CFStringRef path = CFStringCreateWithFileSystemRepresentation(NULL, argv[1]);
            CFArrayRef paths = CFArrayCreate(NULL, (const void**)&path, 1, &kCFTypeArrayCallBacks);
            FSEventStreamRef evts = FSEventStreamCreate(NULL, (FSEventStreamCallback)onFileAdd,
                                                        NULL, paths, kFSEventStreamEventIdSinceNow, 0.5,
                                                        kFSEventStreamCreateFlagFileEvents|kFSEventStreamCreateFlagIgnoreSelf);
            FSEventStreamSetDispatchQueue(evts, queue);
            FSEventStreamStart(evts);
            CFRelease(paths);
            CFRelease(path);
    });
    dispatch_main();
    return 0;
}
