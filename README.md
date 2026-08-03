# CSC-Risk-Evaluation-Tool
A Windows desktop application written in C++ that automates the extraction, comparison, and consolidation of financial information from nonprofit audit PDFs and SAMIS funding spreadsheets.  

The application parses financial reports, aggregates funding data, automatically matches organizations between data sources, allows users to manually resolve unmatched organizations through an interactive interface, and generates a formatted Excel workbook containing the final results. This program is intended for the financial department of the Children's Services Council of Palm Beach County, hereby referred to as CSC.  

## Overview
CSC maintains its financial information across multiple systems. Audit reports from organizations funded by CSC often exist as PDF documents while funding information is stored separately in '.csv' spreadsheets. Because of the quantity of organizations and limitations of standard file storage, this is one of the most slow and error-prone operations reported by the CSC financial department.  

This application streamlines the process by:  
- Extracting financial data from audit PDFs  
- Reading funding information from '.csv' spreadsheets  
- Matching organizations' audit files to spreadsheet entries (included for the possibility of varying naming conventions across platforms)  
- Calculating the values used to determine an organization's overall risk  
- Producing a consolidated Excel report  

The application was designed to reduce manual data entry while maintaining user control over the final matching process.  

## Features
### PDF Processing
- Reads audit reports in PDF format  
- Extracts embedded text using Poppler  
- Automatically falls back to OCR for scanned documents  
- Parses unstructured revenue information from financial statements using a Large Language Model  

### Spreadsheet Processing
- Reads spreadsheet (.csv) files  
- Aggregates funding totals across multiple programs  
- Groups duplicate organizations into a single record  

### Automatic Organization Matching
- Normalizes organization names before comparison  
- Calculates similarity between names  
- Automatically links likely matches  
- Flags ambiguous records for manual review  

### Interactive Match Review
- Displays matched organizations side-by-side  
- Separates unmatched organizations into dedicated panels  
- Supports manual linking of unmatched records  
- Allows users to verify results before generating the final spreadsheet
- Preserves organization relationships established by the user  

### Spreadsheet Generation
- Produces a formatted Excel workbook (.xlsx)  
- Includes funding totals and revenue totals  
- Includes whether an organization has an audit  

## Installation
### Downloading a Release
Download the latest release and extract the contents to a folder. 
Locate the file with extension '.exe' and create a shortcut on your desktop.

Important: Do not move the executable away from the resources folder or required DLL files. The application depends on these files at runtime.  

## Building From Source
### Requirements
- Windows 10 or Windows 11  
- CMake  
- MinGW or another supported C++ compiler  
- vcpkg  
### Required Libraries  
Install dependencies using vcpkg before configuring the project.  

Configure CMake with the vcpkg toolchain, then build using your preferred IDE or command line.  

## Usage
### 1. Select Input Files
Choose:   
- the directory containing organization folders  
- the '.csv' spreadsheet file downloaded from SAMIS  

For CSC employees: The "directory containing organization folders" is "CSC ORGANIZATIONS" within your Box drive. Be sure to have Box synced to your machine.  

### 2. Process Files
The application will:  
- Scan every audit PDF  
- Extract text  
- Perform OCR when necessary  
- Read funding information from the spreadsheet  
- Build organization lists  

Depending on the number of PDFs and whether OCR is required, this step may take several minutes.  

### 3. Review Organization Matches
After processing is complete, a review window is displayed.  

Automatically matched organizations appear together.  

Organizations that could not be confidently matched appear in separate unmatched lists.  

The user can manually associate unmatched organizations and verify automatically matched organizations before continuing.  

### 4. Generate Spreadsheet

Once all desired matches have been verified, generate the final Excel workbook.  

The workbook contains consolidated funding and revenue information for each organization.  
