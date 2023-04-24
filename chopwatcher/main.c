/* -*- c-basic-offset: 4; compile-command: "xcodebuild"; -*- */
//
//  main.c
//  chopwatcher
//
//  Created by Patrick Thomson on 4/21/23.
//

#include <stdio.h>
#include <libgen.h>
#include <CoreServices/CoreServices.h>

static unsigned nthfile = 0; // TODO: a real solution
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

void describe_event(struct FileEvent *evt) {
    printf("Event id %" PRIu64 ":\n", (uint64_t)evt->ident);
    printf("\tpath: %s\n", evt->path);
    printf("\tindex: %zu\n", evt->index);
#define xstr(s) str(s)
#define str(s) #s
#define _GO(name) if (evt->flags & kFSEventStreamEventFlag##name) printf("\t%s\n", xstr(name));
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
#undef GO
#undef str
#undef xstr
}

void onFileAdd(FSEventStreamRef ref,
               void *info,
               size_t numEvents,
               void *eventPathsPtr,
               const FSEventStreamEventFlags *eventFlags,
               const FSEventStreamEventId *eventIds)
{
    char **eventPaths = (char**)eventPathsPtr;
    printf("Event count %zu\n", numEvents);
    const FSEventStreamEventFlags to_ignore
        = kFSEventStreamEventFlagItemInodeMetaMod | kFSEventStreamEventFlagItemModified | kFSEventStreamEventFlagItemRemoved;
    for (size_t ii=0; ii<numEvents; ii++) {
        struct FileEvent evt =
            { .index = ii,
              .path = eventPaths[ii],
              .flags = eventFlags[ii],
              .ident = eventIds[ii],
            };
        if (verbose) describe_event(&evt);
        if (evt.flags & to_ignore) {
            printf("Ignoring...\n");
            continue;
        }
        char *prefix = basename(evt.path);
        char *firstoccurrence = index(prefix, (int)'_') ?: index(prefix, (int)'.');
        if (firstoccurrence != NULL) {
            printf("len: %d\n", (int)(firstoccurrence-prefix));
            prefix[firstoccurrence-prefix] = '\0';
        } else {
            prefix = "";
        }

        char *newpath = NULL;
        asprintf(&newpath, "%s/%03d_%s.wav", dirname(evt.path), nthfile, prefix);
        if (newpath == NULL) die("couldn't build path");
        int ok = rename(eventPaths[ii], newpath);
        fprintf(stderr, "mv %s %s\n", evt.path, newpath);
        nthfile++;
    }
    if (verbose) printf("Done.\n");
}

int main(int argc, const char * argv[])
{
    if (argc != 2)
        die("Error: Directory not provided.");
    CFStringRef path = CFStringCreateWithCString(NULL, argv[1], kCFStringEncodingUTF8);
    CFArrayRef paths = CFArrayCreate(NULL, (const void**)&path, 1, &kCFTypeArrayCallBacks);
    FSEventStreamRef evts = FSEventStreamCreate(NULL, (FSEventStreamCallback)onFileAdd,
                                                NULL, paths, kFSEventStreamEventIdSinceNow, 0.0,
                                                kFSEventStreamCreateFlagFileEvents|kFSEventStreamCreateFlagIgnoreSelf);
#pragma push -Wno-deprecated-declarations
    FSEventStreamScheduleWithRunLoop(evts, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
#pragma pop -Wnodeprecated-declarations
    FSEventStreamStart(evts);
    printf("Booting up…\n");
    CFRunLoopRun();
    CFRelease(paths);
    CFRelease(path);
    return 0;
}
