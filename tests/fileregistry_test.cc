#include <algorithm>

#include <gtest/gtest.h>

#include <bt/bundle/fileregistry.hh>

#include <debug/debug.hh>

using namespace Hypergrace;


class RangeCalculationTest : public ::testing::Test
{
public:
    RangeCalculationTest()
    {
        Bt::FileRegistry::FileList fileList;

        fileList.push_back(std::make_pair("sz1000", 1000));
        fileList.push_back(std::make_pair("sz0000", 0));
        fileList.push_back(std::make_pair("sz0001", 1));
        fileList.push_back(std::make_pair("sz0999", 999));
        fileList.push_back(std::make_pair("sz2000", 2000));
        fileList.push_back(std::make_pair("sz0500", 500));
        fileList.push_back(std::make_pair("sz1500", 1500));

        fileRegistry_ = new Bt::FileRegistry(fileList);
    }

public:
    Bt::FileRegistry *fileRegistry_;
};

TEST_F(RangeCalculationTest, TestRangeCalculation)
{
    {
        Bt::FileRegistry::DataRangeInfo dri;
        fileRegistry_->dataRangeInfo(0, 1000, dri);

        ASSERT_EQ(1, dri.underlyingFiles.size());
        ASSERT_EQ(0, dri.offsetInFirstFile);
        ASSERT_EQ("sz1000", dri.underlyingFiles[0]->filename);
    }

    {
        Bt::FileRegistry::DataRangeInfo dri;
        fileRegistry_->dataRangeInfo(1000, 1, dri);

        ASSERT_EQ(1, dri.underlyingFiles.size());
        ASSERT_EQ(0, dri.offsetInFirstFile);
        ASSERT_EQ("sz0001", dri.underlyingFiles[0]->filename);
    }

    {
        Bt::FileRegistry::DataRangeInfo dri;
        fileRegistry_->dataRangeInfo(0, 1001, dri);

        ASSERT_EQ(2, dri.underlyingFiles.size());
        ASSERT_EQ(0, dri.offsetInFirstFile);
        ASSERT_EQ("sz1000", dri.underlyingFiles[0]->filename);
        ASSERT_EQ("sz0001", dri.underlyingFiles[1]->filename);
    }

    {
        Bt::FileRegistry::DataRangeInfo dri;
        fileRegistry_->dataRangeInfo(999, 2, dri);

        ASSERT_EQ(2, dri.underlyingFiles.size());
        ASSERT_EQ(999, dri.offsetInFirstFile);
        ASSERT_EQ("sz1000", dri.underlyingFiles[0]->filename);
        ASSERT_EQ("sz0001", dri.underlyingFiles[1]->filename);
    }

    {
        Bt::FileRegistry::DataRangeInfo dri;
        fileRegistry_->dataRangeInfo(1000, 2500, dri);

        ASSERT_EQ(3, dri.underlyingFiles.size());
        ASSERT_EQ(0, dri.offsetInFirstFile);
        ASSERT_EQ("sz0001", dri.underlyingFiles[0]->filename);
        ASSERT_EQ("sz0999", dri.underlyingFiles[1]->filename);
        ASSERT_EQ("sz2000", dri.underlyingFiles[2]->filename);
    }

    {
        Bt::FileRegistry::DataRangeInfo dri;
        fileRegistry_->dataRangeInfo(2000, 2000, dri);

        ASSERT_EQ(1, dri.underlyingFiles.size());
        ASSERT_EQ(0, dri.offsetInFirstFile);
        ASSERT_EQ("sz2000", dri.underlyingFiles[0]->filename);
    }

    {
        Bt::FileRegistry::DataRangeInfo dri;
        fileRegistry_->dataRangeInfo(4000, 2000, dri);

        ASSERT_EQ(2, dri.underlyingFiles.size());
        ASSERT_EQ(0, dri.offsetInFirstFile);
        ASSERT_EQ("sz0500", dri.underlyingFiles[0]->filename);
        ASSERT_EQ("sz1500", dri.underlyingFiles[1]->filename);
    }
}
