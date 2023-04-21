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

void die(const char* message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

void onFileAdd(FSEventStreamRef ref,
               void *info,
               size_t numEvents,
               void *eventPathsPtr,
               const FSEventStreamEventFlags *eventFlags,
               const FSEventStreamEventId *eventIds)
{
    char **eventPaths = (char**)eventPathsPtr;
    for (size_t ii=0; ii<numEvents; ii++) {
        char *fullPath = eventPaths[ii];
        FSEventStreamEventFlags flags = eventFlags[ii];
        // in my testing, just checking for item-created is unreliable. weird. who cares.
        bool isCreation = (flags & (kFSEventStreamEventFlagItemRemoved | kFSEventStreamEventFlagItemRenamed)) == 0;
        if (isCreation) {
            char *prefix = basename(fullPath);
            char *firstoccurrence = index(prefix, (int)'_');
            if (firstoccurrence == NULL) {
                prefix = "";
            } else {
                printf("len: %d\n", (int)(firstoccurrence-prefix));
                prefix[firstoccurrence-prefix] = '\0';
            }

            char *newpath = NULL;
            asprintf(&newpath, "%s/%03d_%s.wav", dirname(fullPath), nthfile, prefix);
            if (newpath == NULL) die("couldn't build path");
            int ok = rename(eventPaths[ii], newpath);
            fprintf(stderr, "mv %s %s\n", fullPath, newpath);
            nthfile++;
        }

    }
}

int main(int argc, const char * argv[])
{
    if (argc != 2)
        die("Error: Directory not provided.");
    CFStringRef path = CFStringCreateWithCString(NULL, argv[1], kCFStringEncodingUTF8);
    CFArrayRef paths = CFArrayCreate(NULL, (const void**)&path, 1, &kCFTypeArrayCallBacks);
    FSEventStreamRef evts = FSEventStreamCreate(NULL, (FSEventStreamCallback)onFileAdd,
                                                NULL, paths, kFSEventStreamEventIdSinceNow, 0.01, kFSEventStreamCreateFlagFileEvents);
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
