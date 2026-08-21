# CSC-Risk-Evaluation-Tool
A Windows desktop application written in C++ that automates the extraction, comparison, and consolidation of financial information from nonprofit audit PDFs and CSV funding spreadsheets.

The application parses financial reports, aggregates funding data, automatically matches organizations between data sources, allows users to manually resolve unmatched organizations through an interactive interface, and generates a formatted Excel workbook containing the final results. This program is intended for the financial department of a local government organization, hereby referred to as organization A.

## Overview
Organization A maintains its financial information across multiple systems. Audit reports from organizations funded by organization A often exist as PDF documents while funding information is stored separately in CSV spreadsheets. Because of the quantity of organizations and limitations of standard file storage, this is one of the most slow and error-prone operations reported by the organization A financial department.

This application streamlines the process by:
- Extracting financial data from audit PDFs
- Reading funding information from CSV spreadsheets
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
- Reads spreadsheet (CSV) files
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

Open the config.json file in the release folder. In between the quotation marks, replace "ENTER_API_KEY_HERE" with your API key. Ensure that this is an OpenAI compatible key.  

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
- the CSV spreadsheet file containing funding data

For organization A employees: The "directory containing organization folders" is "[organization_A] ORGANIZATIONS" within your Box drive. Be sure to have Box synced to your machine.

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

## How It Works
### Audit Processing
- Scan through the organization folders and identify the audit report for the desired year  
- Load each PDF  
- Extract embedded text using Poppler  
- Detect documents with little or no extractable text  
- Perform OCR using Tesseract when necessary  
- Gather pages that likely contain the total revenue  
- Send all pages to the LLM in a single prompt per organization to extract the total revenue  

### Spreadsheet Processing
- The CSV spreadsheet is parsed with vincentlaucsb-csv-parser to identify organizations and their associated funding  
- If multiple programs belong to the same organization, their funding totals are combined into a single record  

### Organization Matching
- Organization names are first normalized by removing common formatting differences such as punctuation and common suffixes  
- The application then compares normalized names using a similarity algorithm to identify likely matches
- Organizations exceeding the similarity threshold are matched automatically  
- Remaining organizations are presented to the user for manual review  
- The final matches are saved in an unordered map and serialized into a file in the resources directory
- If the file storing the serialized unordered map exists, the application will load any eligible matches before the matching algorithm runs

### Report Generation
- After all matches have been finalized, the application creates a new Excel workbook containing the consolidated financial information  
- If the application has been run before, the existing workbook is appended to in a new sheet  

## Technologies Used
### SFML
Used to create the graphical user interface.  

Responsibilities include:  
- window creation  
- rendering  
- text drawing  
- mouse and keyboard input  
- scrolling interface components  

### Vincentlaucsb csv parser
Used to extract values from the .csv file submitted as input.

### OpenXLSX
Used for reading and writing Excel workbooks.  

Responsibilities include:
- generating the final report  
- formatting spreadsheet contents  

### Poppler
Used for PDF text extraction.  

Responsibilities include:
- opening PDF documents
- extracting page text
- providing access to document metadata

### Tesseract OCR
Used only when embedded PDF text is unavailable.  

Responsibilities include:  
- optical character recognition  
- recovering text from scanned documents  

### nlohmann/json
Used for parsing JSON data exchanged with the language model.  

### Windows API
Used for platform-specific functionality. including native dialog boxes and operating system interaction.  

Responsibilities include:  
- native dialog boxes  
- windows filepicker  

### CMake
Used to configure and build the project.  

### vcpkg
Used for dependency management.

## Limitations
Currently supports Windows only.  

OCR processing significantly increases runtime.  

Input documents are expected to follow supported audit and spreadsheet formats.  

Organization names with significant differences across platforms may require manual matching.  

Processing time increases with the number and size of PDF documents.  

The application currently requires the path to an organization's audit directory to be under 250 characters. Organizations stored in deeper directory structures may not be processed successfully.  
