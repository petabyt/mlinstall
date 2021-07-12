#ifndef DRIVE_H
#define DRIVE_H

// Get a usable drive directory, for
// making files and stuff
void getUsableDrive(char buffer[]);

// Disable/enable card flags
int enableFlag();
int disableFlag();

#endif