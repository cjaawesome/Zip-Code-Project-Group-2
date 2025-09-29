/**
 * @file CSVBuffer.cpp
 * @author Group 2 
 * @brief Implementation of CSVBuffer class for reading zip code CSV files
 * @version 0.1
 * @date 2025-09-17
 */

#include "CSVBuffer.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>

/**
 * @brief Default constructor
 * @details Initializes buffer in closed state
 */
CSVBuffer::CSVBuffer() 
    : lineNumber(0), recordsProcessed(0), errorState(false), lastError("")
{
}

/**
 * @brief Constructor with filename
 * @details Automatically opens file and skips header
 */
CSVBuffer::CSVBuffer(const std::string& filename)
    : lineNumber(0), recordsProcessed(0), errorState(false), lastError("")
{
    openFile(filename);
}

/**
 * @brief Destructor
 * @details Automatically closes file if open
 */
CSVBuffer::~CSVBuffer()
{
    closeFile();
}

/**
 * @brief Open CSV file for reading
 * @details Opens file, validates, and skips header
 */
bool CSVBuffer::openFile(const std::string& filename)
{
    closeFile();  // Close any existing file
    
    csvFile.open(filename); //open files
    if (!csvFile.is_open()) //if file couldn't be opened
    {
        setError("Could not open file: " + filename); //flag error
        return false; //flag error
    }
    
    lineNumber = 0; // sets line number to 0 representing being at the begining at the file
    recordsProcessed = 0; //sets recordsProcessed to 0 meaning no records are processed
    errorState = false; // sets error state to false because no error has happened yet
    lastError = ""; 
    
    // Skip and validate header
    if (!skipHeader()) 
    {
        closeFile();
        return false;
    }
    
    return true;
}

/**
 * @brief Read next zip code record from file
 * @details Parses line and converts to ZipCodeRecord
 */
bool CSVBuffer::getNextRecord(ZipCodeRecord& record)
{
    if (!csvFile.is_open() || errorState) 
    {
        return false;
    }
    
    // Check if we have a line stored from skipHeader()
    if (!currentLine.empty()) 
    {
        ++lineNumber; // Use the stored line
        std::string lineToProcess = currentLine;
        currentLine.clear(); // Clear it so we don't use it again
        
        std::vector<std::string> fields;
        if (!parseLine(lineToProcess, fields)) // Parse this line
        {
            setError("Failed to parse line " + std::to_string(lineNumber));
            return false;
        }
        
        if (!fieldsToRecord(fields, record)) // Convert fields to record
        {
            setError("Failed to convert fields to record on line " + std::to_string(lineNumber));
            return false;
        }
        
        ++recordsProcessed;
        return true;
    }
    
    if (csvFile.eof())  // Normal reading from file
    {
        return false;
    }
    
    if (!std::getline(csvFile, currentLine)) 
    {
        return false;  // End of file or read error
    }
    
    ++lineNumber;
    
    if (currentLine.empty()) // Skip empty lines
    {
        return getNextRecord(record);  // Recursively try next line
    }
    
    std::vector<std::string> fields;
    if (!parseLine(currentLine, fields)) // Parse the line into fields
    {
        setError("Failed to parse line " + std::to_string(lineNumber));
        return false;
    }
    
    if (!fieldsToRecord(fields, record))  // Convert fields to record
    {
        setError("Failed to convert fields to record on line " + std::to_string(lineNumber));
        return false;
    }
    
    ++recordsProcessed;
    currentLine.clear(); // Clear the line buffer
    return true;
}

/**
 * @brief Check if more records are available
 */
bool CSVBuffer::hasMoreRecords() const
{
    return csvFile.is_open() && !csvFile.eof() && !errorState;
}

/**
 * @brief Close the CSV file
 */
void CSVBuffer::closeFile()
{
    if (csvFile.is_open()) //if file is open close the file
    {
        csvFile.close(); 
    }
    //reset flags
    lineNumber = 0;
    recordsProcessed = 0;
    errorState = false;
    lastError = "";
}

// Getter methods
int CSVBuffer::getCurrentLineNumber() const
{
    return lineNumber;
}

int CSVBuffer::getRecordsProcessed() const
{
    return recordsProcessed;
}

bool CSVBuffer::hasError() const
{
    return errorState;
}

std::string CSVBuffer::getLastError() const
{
    return lastError;
}

/**
 * @brief Skip and validate CSV header row
 * @details Handles multi-line quoted headers in the given Zip Code format
 */
bool CSVBuffer::skipHeader()
{
    if (!csvFile.is_open()) //if file isn't open flag error and return false
    {
        setError("File not open");
        return false;
    }
    
    std::string line;
    while (std::getline(csvFile, line)) 
    {
        ++lineNumber;
        
        trimString(line);
        
        if (line.empty())  // Skip empty lines
        {
            continue;
        }
        
        if (!line.empty() && std::isdigit(line[0]))  // Check if line starts with a number (zip code data)
        {
            currentLine = line;  // ZipCode data found store it for the next getNextRecord() call
            --lineNumber; // Increment it again in getNextRecord
            return true;
        }
        // Still in header continue looping
    }
    setError("No data rows found in file");  // No data found
    return false;
}

/**
 * @brief Parse comma-separated line into fields
 * @details Simple CSV parsing for your data format
 */
bool CSVBuffer::parseLine(const std::string& line, std::vector<std::string>& fields)
{
    fields.clear();
    std::stringstream ss(line);
    std::string field;
    
    while (std::getline(ss, field, ',')) 
    {
        trimString(field);
        fields.push_back(field);
    }
    
    if (fields.size() != EXPECTED_FIELD_COUNT) // Validate field count
    {
        return false;
    }
    
    return true;
}

/**
 * @brief Convert string fields to ZipCodeRecord
 * @details Validates and converts each field type
 */
bool CSVBuffer::fieldsToRecord(const std::vector<std::string>& fields, ZipCodeRecord& record)
{
    if (fields.size() != EXPECTED_FIELD_COUNT) 
        return false;
    
    // Validate and convert each field
    // Field 0: Zip Code (integer)
    if (!isValidInteger(fields[0])) 
    {
        return false;
    }
    int zipCode = std::stoi(fields[0]);
    
    // Field 4: Latitude (double)
    if (!isValidDouble(fields[4])) 
    {
        return false;
    }
    double latitude = std::stod(fields[4]);
    
    // Field 5: Longitude (double)  
    if (!isValidDouble(fields[5])) 
    {
        return false;
    }
    double longitude = std::stod(fields[5]);
    
    // Field 1: Place Name (string)
    std::string placeName = fields[1];
    
    // Field 2: State (string, must be 2 chars)
    std::string state = fields[2];
    if (state.length() != 2)
    {
        return false;
    }
    
    // Field 3: County (string)
    std::string county = fields[3];
    
    // Create the record
    ZipCodeRecord tempRecord(zipCode, latitude, longitude, placeName, state, county);
    record = tempRecord;
    
    return true;
}

/**
 * @brief Trim whitespace from string
 */
void CSVBuffer::trimString(std::string& str)
{
    // Remove leading whitespace
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) 
    {
        return !std::isspace(ch);
    }));
    
    // Remove trailing whitespace  
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) 
    {
        return !std::isspace(ch);
    }).base(), str.end());
}

/**
 * @brief Validate that string represents valid integer
 */
bool CSVBuffer::isValidInteger(const std::string& str) const
{
    if (str.empty()) return false; //if string is empty, return false
    
    try //try to convert string to integer, return true if successfull
    {
        std::stoi(str);
        return true;
    } 
    catch (const std::exception&) 
    {
        return false;
    }
}

/**
 * @brief Validate that string represents valid double
 */
bool CSVBuffer::isValidDouble(const std::string& str) const
{
    if (str.empty()) return false; //if string is empty, return false
    
    try //try to convert string to double, return true if successfull
    {
        std::stod(str);
        return true;
    } 
    catch (const std::exception&) 
    {
        return false;
    }
}

/**
 * @brief Set error state with message
 */
void CSVBuffer::setError(const std::string& message)
{
    //set error flags
    errorState = true;
    lastError = message;
}