#include "PDFParser.hpp"
#include <iostream>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-image.h>

/*
 * OCR FUNCTIONALITY
*/

#include <poppler/cpp/poppler-page-renderer.h>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <filesystem>

static std::string performOCR(poppler::page *page) {
    poppler::page_renderer renderer;

    // Render PDF page to image at 300 DPI
    poppler::image img = renderer.render_page(page, 300, 300);

    if (!img.is_valid()) {
        std::cerr << "Failed to render page for OCR\n";
        return {};
    }

    // Save temporary image
    std::filesystem::path tempImage = std::filesystem::current_path() / "Instance_Storage" / "ocr_page.png";

    img.save(tempImage.string(), "png");

    tesseract::TessBaseAPI api;

    if (api.Init("tessdata", "eng") != 0) {
        std::cerr << "Failed to initialize Tesseract\n";
        return {};
    }

    Pix *pix = pixRead(tempImage.string().c_str());

    if (!pix) {
        std::cerr << "Failed to load OCR image\n";
        api.End();
        return {};
    }

    api.SetImage(pix);

    char *output = api.GetUTF8Text();

    std::string text;

    if (output) {
        text = output;
        delete[] output;
    }

    pixDestroy(&pix);

    api.End();

    // Remove temporary image
    std::error_code ec;
    std::filesystem::remove(tempImage, ec);

    return text;
}

std::vector<AuditPage> PDFParser::extractRelevantPages(const std::filesystem::path &localPDF, const bool &OCR) {
    std::vector<AuditPage> pages;

    if (localPDF.empty()) {
        std::cerr << "Could not create local PDF copy\n";
        return {};
    }

    poppler::document *document = poppler::document::load_from_file(localPDF.string());

    if (!document) {
        std::cerr << "Failed to open PDF: "
                << localPDF
                << "\n";

        return pages;
    }


    int pageCount = document->pages();


    for (int i = 0; i < pageCount; i++) {
        poppler::page *page = document->create_page(i);

        if (!page)
            continue;


        auto text = page->text();
        auto utf8 = text.to_utf8();
        std::string pageText(utf8.begin(), utf8.end());


        /*
         * OCR FUNCTIONALITY
        */
        // here, specify a range of pages that it could be within so we don't have to OCR 50 pages
        // when we know its between like 5-15
        if (OCR && pageText.size() <= 5 && i >= 2 && i <= 15) // very likely an image instead of text
        {
            // OCR fallback
            std::cout << "Running OCR..." << std::endl;
            pageText = performOCR(page);
        }

        if (
            (pageText.find("TABLE OF CONTENTS") == std::string::npos &&
             pageText.find("Table of Contents") == std::string::npos)
            &&
            (pageText.find("ACTIVITIES") != std::string::npos ||
             pageText.find("Activities") != std::string::npos)
            &&
            (pageText.find("Cash Flows") == std::string::npos &&
             pageText.find("CASH FLOWS") == std::string::npos)
            &&
            (pageText.find("NOTES TO THE FINANCIAL STATEMENTS") == std::string::npos)
        ) {
            AuditPage auditPage;

            auditPage.pageNumber = i + 1;
            auditPage.text = pageText;

            pages.push_back(std::move(auditPage));
        }
        delete page;
    }
    delete document;

    return pages;
}
