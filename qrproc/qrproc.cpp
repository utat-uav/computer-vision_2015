#include <opencv2/opencv.hpp>

void readQR(cv::Mat &roi)
{
    cv::Mat gryscl;
    cv::cvtColor(roi,gryscl,CV_RGB2GRAY);
    unsigned char* data = (unsigned char*)(gryscl.data);

    zbar::Image im(gryscl.cols, gryscl.rows, "Y800", data, gryscl.cols*gryscl.rows);
    zbar::ImageScanner scanner;
    scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE,1);
    scanner.scan(im);
    for(zbar::Image::SymbolIterator symbol = im.symbol_begin(); symbol!=im.symbol_end(); ++symbol)
    {
        std::cout<<"Found QR Code: " <<symbol->get_data()<<std::endl;
    }
}

int main ()
