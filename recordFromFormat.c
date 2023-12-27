/*
 * This file implements two functions that read XML and binary information from a buffer,
 * respectively, and return pointers to Record or NULL.
 *
 * *** YOU MUST IMPLEMENT THESE FUNCTIONS ***
 *
 * The parameters and return values of the existing functions must not be changed.
 * You can add function, definition etc. as required.
 */
#include "recordFromFormat.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#define MAXGRADES 4
#define MAXCOURSES 11

static const struct {
    Grade grade;
    const char *str;
} grade_conversion[] = {
    {Grade_None, "None"},
    {Grade_Bachelor, "Bachelor"},
    {Grade_Master, "Master"},
    {Grade_PhD, "PhD"},
};

static const struct {
    int course;
    const char *str;
} course_conversion[] = {
    {Course_IN1000, "IN1000"},
    {Course_IN1010, "IN1010"},
    {Course_IN1020, "IN1020"},
    {Course_IN1030, "IN1030"},
    {Course_IN1050, "IN1050"},
    {Course_IN1060, "IN1060"},
    {Course_IN1080, "IN1080"},
    {Course_IN1140, "IN1140"},
    {Course_IN1150, "IN1150"},
    {Course_IN1900, "IN1900"},
    {Course_IN1910, "IN1910"},
};

Record *XMLtoRecord(char *buffer, int bufSize, int *bytesread) {
    Record *r = NULL;
    char deli[] = "<>";
    char  *rest = NULL, *token;
    bool error = false, building_course_member = false;
    //    strcpy(xml, buffer);
    // Parse the XML string

    for (token = strtok_r(buffer, deli, &rest); strcmp(token, "/record");
         token = strtok_r(NULL, deli, &rest)) {
        // if (token == NULL)
        //     break;

        if (token[0] == '\n')
            continue;

        char *tag = strtok_r(token, "=", &token);
        char *value = strtok_r(NULL, "\"\"", &token);

        // value is needed for non list tags and if there is no tag xml is invalid
        if (value == NULL)
            if (strcmp(tag, "record") && strstr(tag, "courses") == NULL) {
                error = true;
                break;
            }

        // if the next tag is non course element while parsing courses tree
        // means this xmls i invalid
        if (building_course_member && !strstr(tag, "course")) {
            error = true;
            break;
        }

        if (strcmp(tag, "record") == 0) {
            if (!r) {
                r = newRecord();
            } else {
                // it's possible to create a empty record
                error = false;
            }
        } else if (strcmp(tag, "source") == 0) {
            setSource(r, value[0]);
            *bytesread += 1;
        } else if (strcmp(tag, "dest") == 0) {
            setDest(r, value[0]);
            *bytesread += 1;
        } else if (strcmp(tag, "username") == 0) {
            setUsername(r, value);
            *bytesread += strlen(value);
        } else if (strcmp(tag, "id") == 0) {
            setId(r, atoi(value));
            *bytesread += strlen(value);
        } else if (strcmp(tag, "group") == 0) {
            setGroup(r, atoi(value));
            *bytesread += strlen(value);
        } else if (strcmp(tag, "semester") == 0) {
            setSemester(r, atoi(value));
            *bytesread += strlen(value);
        } else if (strcmp(tag, "grade") == 0) {
            // find the correct grade from sting
            for (int i = 0; i < MAXGRADES; i++)
                if (strcmp(value, grade_conversion[i].str) == 0) {
                    setGrade(r, grade_conversion[i].grade);
                    *bytesread += strlen(value);
                    break;
                }
        } else if (strcmp(tag, "courses") == 0 || strcmp(tag, "/courses") == 0) {
            // init a course memeber
            if (!r->has_courses) {
                setCourse(r, 0);
                building_course_member = true;
            } else if (r->courses > 0 && building_course_member) {
                building_course_member = false;
            } else {
                // record shoudl at least have one course if <courses> tag is
                // found
                error = true;
                break;
            }
        } else if (strcmp(tag, "course") == 0) {
            // find the correct enum cource from sting
            bool found_course = false;
            for (int i = 0; i < MAXCOURSES; i++) {
                if (strcmp(value, course_conversion[i].str) == 0) {
                    setCourse(r, course_conversion[i].course);
                    *bytesread += strlen(value);
                    found_course = true;
                }
            }
            if (!found_course) {
                error = true;
                break;
            }
        } else {
            // unrecognised tag was found
            error = true;
            break;
        }
    }

    if (error) {
        deleteRecord(r);
        return NULL;
    }

    strcpy(buffer, rest);
    return r;
}

Record *BinaryToRecord(char *buffer, int bufSize, int *bytesread) {
    /* TO BE IMPLEMENTED */
    uint8_t flags = buffer[0];
    if (flags == 0) // Only the flag is required
        return NULL;

    Record *r = newRecord();
    initRecord(r);
    int sz = 1;

    if (flags & FLAG_SRC) {
        setSource(r, buffer[sz]);
        sz += 1;
    }

    if (flags & FLAG_DST) {
        setDest(r, buffer[sz]);
        sz += 1;
    }
    if (flags & FLAG_USERNAME) {
        uint32_t len;
        memcpy(&len, &buffer[sz], 4);
        len = ntohl(len); // Convert to network byte order
        sz += 4;

        char *username = malloc(sizeof(char) * (len + 1));

        memcpy(username, &buffer[sz], len);
        username[len] = '\0';
        setUsername(r, username);
        free(username);
        sz += (int)len;
    }
    if (flags & FLAG_ID) {
        uint32_t id = 0;
        memcpy(&id, &buffer[sz], 4);
        id = ntohl(id); // Convert to network byte order
        setId(r, id);
        sz += 4;
    }
    if (flags & FLAG_GROUP) {
        uint32_t group = 0;
        memcpy(&group, &buffer[sz], 4);
        group = ntohl(group); // Convert to network byte order
        setGroup(r, group);
        sz += 4;
    }
    if (flags & FLAG_SEMESTER) {
        uint8_t semester;
        memcpy(&semester, &buffer[sz], 1);
        setSemester(r, semester);
        sz += 1;
    }
    if (flags & FLAG_GRADE) {
        uint8_t ordinal;
        memcpy(&ordinal, &buffer[sz], 1);
        Grade grade = (Grade)ordinal;
        setGrade(r, grade);
        sz += 1;
    }
    if (flags & FLAG_COURSES) {
        uint16_t course_flag;
        memcpy(&course_flag, &buffer[sz], 2);
        course_flag = ntohs(course_flag); // Convert to network byte order
        for (int i = 0; i < MAXCOURSES; i++) {
            if (course_flag & course_conversion[i].course)
                setCourse(r, course_conversion[i].course);
        }
        sz += 2;
    }
    *bytesread = sz;

    return r;
}
