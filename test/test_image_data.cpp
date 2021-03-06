#include <vector>

#include "image/image_data.h"
#include "util/test_util.h"

#include "opencv2/core/core.hpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using super_resolution::ImageData;
using super_resolution::test::AreImagesEqual;
using super_resolution::test::AreMatricesEqual;

constexpr double kPixelErrorTolerance = 1.0 / 255.0;

// Test color channels.
static const cv::Mat kTestChannelB = (cv::Mat_<double>(4, 4)
    << 0.1,  0.2,  0.3,  0.4,
       0.15, 0.25, 0.35, 0.45,
       0.55, 0.75, 0.85, 0.95,
       0.6,  0.65, 0.7,  0.75);
static const cv::Mat kTestChannelG = (cv::Mat_<double>(4, 4)
    << 0.2,  0.3,  0.4, 0.45,
       0.1,  0.2,  0.3, 0.4,
       0.75, 0.65, 1.0, 1.0,
       0.3,  0.35, 0.4, 0.45);
static const cv::Mat kTestChannelR = (cv::Mat_<double>(4, 4)
    << 0.0,  0.05, 0.1,  0.1,
       0.0,  0.0,  0.05, 0.1,
       0.25, 0.1,  0.2,  0.2,
       0.0,  0.05, 0.1,  0.15);
static const std::vector<cv::Mat> kTestColorChannels =
    {kTestChannelB, kTestChannelG, kTestChannelR};

// This test verifies that channels are added correctly to ImageData and pixels
// and channels in the image can be accessed and manipulated correctly.
TEST(ImageData, AddAndAccessImageData) {
  const int num_test_rows = 3;
  const int num_test_cols = 5;

  ImageData image_data;

  /* Verify behavior of a an empty image. */

  EXPECT_EQ(image_data.GetNumChannels(), 0);
  EXPECT_EQ(image_data.GetImageSize(), cv::Size(0, 0));
  EXPECT_EQ(image_data.GetNumPixels(), 0);

  /* Verify behavior of a single-channel image. */

  // Convert to CV_8UC1 (unsigned char) 0-255 range image first.
  cv::Mat channel_0 = (cv::Mat_<double>(num_test_rows, num_test_cols)
      << 0.1,  0.2,  0.3,  0.4,  0.5,
         0.15, 0.25, 0.35, 0.45, 0.55,
         0.6,  0.65, 0.7,  0.75, 0.8);
  cv::Mat channel_0_original_clone = channel_0.clone();
  cv::Mat channel_0_converted;
  channel_0.convertTo(channel_0_converted, CV_8UC1, 255);
  image_data.AddChannel(channel_0_converted);

  EXPECT_EQ(image_data.GetNumChannels(), 1);
  EXPECT_EQ(image_data.GetImageSize(), cv::Size(num_test_cols, num_test_rows));
  EXPECT_EQ(image_data.GetNumPixels(), num_test_rows * num_test_cols);

  // Check pixel access values.
  EXPECT_NEAR(image_data.GetPixelValue(0, 0), 0.1, kPixelErrorTolerance);
  EXPECT_NEAR(image_data.GetPixelValue(0, 2), 0.3, kPixelErrorTolerance);
  EXPECT_NEAR(image_data.GetPixelValue(0, 8), 0.45, kPixelErrorTolerance);
  EXPECT_NEAR(image_data.GetPixelValue(0, 11), 0.65, kPixelErrorTolerance);

  // Check that another ImageData created with pre-normalized values (between 0
  // and 1 instead of between 0 and 255) will have identical data.
  ImageData image_data2(channel_0);  // channel_0 is NOT converted.
  EXPECT_TRUE(AreImagesEqual(image_data, image_data2, kPixelErrorTolerance));

  // Check that the returned channel image matches.
  cv::Mat returned_channel_0 = image_data.GetChannelImage(0);
  EXPECT_TRUE(AreMatricesEqual(
      returned_channel_0, channel_0, kPixelErrorTolerance));

  // Check data pointer access.
  double* pixel_ptr = image_data.GetMutableChannelData(0);
  EXPECT_NEAR(pixel_ptr[0], 0.1, kPixelErrorTolerance);
  EXPECT_NEAR(pixel_ptr[3], 0.4, kPixelErrorTolerance);
  EXPECT_NEAR(pixel_ptr[4], 0.5, kPixelErrorTolerance);
  EXPECT_NEAR(pixel_ptr[14], 0.8, kPixelErrorTolerance);

  // Check data manipulation through the pointers works as expected.
  // Change all pixel values to 0.33 and expect the image to be updated.
  const double new_pixel_value = 0.33;
  for (int i = 0; i < num_test_rows * num_test_cols; ++i) {
    pixel_ptr[i] = new_pixel_value;
  }
  // Check that all returned pixel values are updated.
  for (int i = 0; i < num_test_rows * num_test_cols; ++i) {
    EXPECT_NEAR(
        image_data.GetPixelValue(0, i), new_pixel_value, kPixelErrorTolerance);
  }
  // Check that the returned channel image has also been updated.
  returned_channel_0 = image_data.GetChannelImage(0);
  for (int row = 0; row < num_test_rows; ++row) {
    for (int col = 0; col < num_test_cols; ++col) {
      EXPECT_NEAR(
          returned_channel_0.at<double>(row, col),
          new_pixel_value,
          kPixelErrorTolerance);
    }
  }

  // Check that the image channel_0_converted which got inserted as a channel
  // got inserted as a copy and that the original image was not actually
  // modified.
  cv::Mat channel_0_clone_converted;
  channel_0_original_clone.convertTo(channel_0_clone_converted, CV_8UC1, 255);
  EXPECT_TRUE(AreMatricesEqual(
      channel_0_converted, channel_0_clone_converted));

  /* Verify behavior with multiple channels. */

  // Add 10 more channels.
  for (int i = 0; i < 10; ++i) {
    const double pixel_value = 1.0 / static_cast<double>(i+1);
    cv::Mat next_channel(num_test_rows, num_test_cols, CV_64FC1);
    next_channel = cv::Scalar(pixel_value);
    cv::Mat next_channel_converted;
    next_channel.convertTo(next_channel_converted, CV_8UC1, 255);
    image_data.AddChannel(next_channel_converted);
  }

  EXPECT_EQ(image_data.GetNumChannels(), 11);
  EXPECT_EQ(image_data.GetImageSize(), cv::Size(num_test_cols, num_test_rows));
  EXPECT_EQ(image_data.GetNumPixels(), num_test_rows * num_test_cols);

  // TODO: check that we can access pixels in each channel.
  // TODO: check that we can access the data pointer in each channel.
  // TODO: check that we can manipulate the data pointer in each channel.
}

// Checks that adding an image using the AddChannel with array method works.
TEST(ImageData, AddChannelArray) {
  const double pixel_values[20] = {
       0.1,  0.2,  0.3,  0.4,  0.5,
      0.15, 0.25, 0.35, 0.45, 0.55,
      0.55, 0.75, 0.85, 0.95, 1.05,
      -0.3, 0.6,  0.65, 0.7,  0.75
  };
  const cv::Mat expected_channel = (cv::Mat_<double>(4, 5)
    << 0.1,  0.2,  0.3,  0.4,  0.5,
       0.15, 0.25, 0.35, 0.45, 0.55,
       0.55, 0.75, 0.85, 0.95, 1.05,
       -0.3, 0.6,  0.65, 0.7,  0.75);
  const cv::Size image_size(5, 4);

  // Test that the method works if adding a channel to an empty image.
  ImageData image_1;
  image_1.AddChannel(pixel_values, image_size);
  EXPECT_EQ(image_1.GetNumChannels(), 1);
  EXPECT_EQ(image_1.GetNumPixels(), 20);
  EXPECT_EQ(image_1.GetImageSize(), image_size);
  EXPECT_TRUE(AreMatricesEqual(image_1.GetChannelImage(0), expected_channel));

  // Test that the method works if appending a channel to a non-empty image.
  ImageData image_2(expected_channel, super_resolution::DO_NOT_NORMALIZE_IMAGE);
  image_2.AddChannel(pixel_values, image_size);
  EXPECT_EQ(image_2.GetNumChannels(), 2);
  EXPECT_EQ(image_2.GetNumPixels(), 20);
  EXPECT_EQ(image_2.GetImageSize(), image_size);
  EXPECT_TRUE(AreMatricesEqual(image_2.GetChannelImage(1), expected_channel));
}

// Checks that the ImageData(const double*, const cv::Size&) constructor
// correctly builds the ImageData from the pixel value array and copies the
// data so that modifying the ImageData won't change the original array.
TEST(ImageData, PixelArrayConstructor) {
  /* Verify functionality with a single channel. */

  // Not const because we're testing that the data is actually copied.
  double pixel_values[9] = {
     1.0, 0.5, 0.9,
     100,   0, -50,
    -0.1, 0.0,   1
  };
  const cv::Size size(3, 3);
  ImageData image_data(pixel_values, size);

  EXPECT_EQ(image_data.GetNumChannels(), 1);
  EXPECT_EQ(image_data.GetImageSize(), cv::Size(3, 3));
  EXPECT_EQ(image_data.GetNumPixels(), 9);

  // Make sure that the data is identical.
  for (int i = 0; i < 9; ++i) {
    EXPECT_DOUBLE_EQ(image_data.GetPixelValue(0, i), pixel_values[i]);
  }

  // Make sure that changing the image doesn't change the original data.
  double* image_data_ptr = image_data.GetMutableChannelData(0);
  image_data_ptr[0] = 0.0;
  image_data_ptr[3] = 1.0;
  image_data_ptr[8] = -500;

  EXPECT_EQ(pixel_values[0], 1.0);
  EXPECT_EQ(pixel_values[3], 100);
  EXPECT_EQ(pixel_values[8], 1);

  /* Verify this all still works with multiple image channels. */

  double pixel_values_multichannel[9 * 4] = {
    // Channel 1 (same as before):
     1.0, 0.5, 0.9,
     100,   0, -50,
    -0.1, 0.0,   1,
    // Channel 2:
    10, 20, 30,
    40, 50, 60,
    70, 80, 90,
    // Channel 3:
    1, 2, 3,
    4, 5, 6,
    7, 8, 9,
    // Channel 4:
    0.1, 0.2, 0.3,
    0.4, 0.5, 0.6,
    0.7, 0.8, 0.9
  };
  ImageData image_data_multichannel(pixel_values_multichannel, size, 4);

  EXPECT_EQ(image_data_multichannel.GetNumChannels(), 4);
  EXPECT_EQ(image_data_multichannel.GetImageSize(), cv::Size(3, 3));
  EXPECT_EQ(image_data_multichannel.GetNumPixels(), 9);

  // Make sure that the data is identical.
  for (int channel_index = 0; channel_index < 4; ++channel_index) {
    for (int pixel_index = 0; pixel_index < 9; ++pixel_index) {
      const int array_index = channel_index * 9 + pixel_index;
      EXPECT_DOUBLE_EQ(
          image_data_multichannel.GetPixelValue(channel_index, pixel_index),
          pixel_values_multichannel[array_index]);
    }
  }

  // Make sure that changing the image doesn't change the original data.
  image_data_ptr = image_data.GetMutableChannelData(0);
  image_data_ptr[3] = 1.0;
  EXPECT_EQ(pixel_values_multichannel[3], 100);

  image_data_ptr = image_data_multichannel.GetMutableChannelData(1);
  image_data_ptr[5] = -500;
  EXPECT_EQ(pixel_values_multichannel[9 + 5], 60);

  image_data_ptr = image_data_multichannel.GetMutableChannelData(2);
  image_data_ptr[8] = 25.3;
  EXPECT_EQ(pixel_values_multichannel[18 + 8], 9);

  image_data_ptr = image_data_multichannel.GetMutableChannelData(3);
  image_data_ptr[0] = -10;
  EXPECT_EQ(pixel_values_multichannel[27 + 0], 0.1);
}

// This test verifies that the copy constructor works as expected.
TEST(ImageData, CopyConstructor) {
  // Create an ImageData object with 10 channels.
  ImageData image_data;
  for (int i = 0; i < 10; ++i) {
    cv::Mat next_channel(25, 25, CV_8UC1);
    next_channel = cv::Scalar(5 * i);  // single intensity is 5 * i
    image_data.AddChannel(next_channel);
  }

  // Run some standard checks.
  EXPECT_EQ(image_data.GetNumChannels(), 10);
  EXPECT_EQ(image_data.GetImageSize(), cv::Size(25, 25));
  EXPECT_EQ(image_data.GetNumPixels(), 25 * 25);

  // Copy the ImageData and verify that the new object matches the old object.
  ImageData image_data2 = image_data;
  EXPECT_EQ(image_data2.GetNumChannels(), 10);
  EXPECT_EQ(image_data2.GetImageSize(), cv::Size(25, 25));
  EXPECT_EQ(image_data2.GetNumPixels(), 25 * 25);

  EXPECT_TRUE(AreImagesEqual(image_data, image_data2));

  // Check that the new ImageData is a clone, and changing the data will not
  // affect the old ImageData object.
  // TODO: do this.
}

// This test verifies that the constructor which takes an OpenCV image as input
// works as expected, and correctly splits up the channels.
TEST(ImageData, FromOpenCvImageConstructor) {
  // TODO: implement.
  // ImageData(const cv::Mat& image)

  /* Verify the functionality of the manual normalization constructor. */

  const cv::Mat invalid_image = (cv::Mat_<double>(3, 3)
      << 0.5, 1.5,  100,
         -25, 0.0,  -30,
         55,  1.98, 1000);
  ImageData image_data_not_normalized(
      invalid_image, super_resolution::DO_NOT_NORMALIZE_IMAGE);
  for (int i = 0; i < 9; ++i) {
    EXPECT_DOUBLE_EQ(
        invalid_image.at<double>(i),
        image_data_not_normalized.GetPixelValue(0, i));
  }
}

// This test verifies that the image is correctly resized, with one or more
// channels.
TEST(ImageData, ResizeImage) {
  const cv::Mat image_pixels = (cv::Mat_<double>(4, 4)
      << 0.1, 0.2, 0.3, 0.4,
         0.5, 0.6, 0.7, 0.8,
         0.9, 1.0, 0.0, 0.2,
         0.4, 0.6, 0.8, 1.0);
  ImageData image;
  const int num_channels = 10;
  for (int channel_index = 0; channel_index < num_channels; ++channel_index) {
    image.AddChannel(image_pixels);
  }

  /* Verify that downsampling works. */

  const cv::Mat expected_smaller_image = (cv::Mat_<double>(2, 2)
      << 0.1, 0.3,
         0.9, 0.0);
  // Try with cv::Size(2, 2).
  ImageData smaller_image_1 = image;  // copy
  smaller_image_1.ResizeImage(
      cv::Size(2, 2), super_resolution::INTERPOLATE_NEAREST);
  // Try with scale factor of 0.5.
  ImageData smaller_image_2 = image;
  smaller_image_2.ResizeImage(0.5, super_resolution::INTERPOLATE_NEAREST);
  // Check results.
  for (int channel_index = 0; channel_index < num_channels; ++channel_index) {
    EXPECT_TRUE(AreMatricesEqual(
        smaller_image_1.GetChannelImage(channel_index),
        expected_smaller_image));
    EXPECT_TRUE(AreMatricesEqual(
        smaller_image_2.GetChannelImage(channel_index),
        expected_smaller_image));
  }

  /* Verify that upsampling works. */

  const cv::Mat expected_bigger_image = (cv::Mat_<double>(8, 8)
      << 0.1, 0.1, 0.2, 0.2, 0.3, 0.3, 0.4, 0.4,
         0.1, 0.1, 0.2, 0.2, 0.3, 0.3, 0.4, 0.4,
         0.5, 0.5, 0.6, 0.6, 0.7, 0.7, 0.8, 0.8,
         0.5, 0.5, 0.6, 0.6, 0.7, 0.7, 0.8, 0.8,
         0.9, 0.9, 1.0, 1.0, 0.0, 0.0, 0.2, 0.2,
         0.9, 0.9, 1.0, 1.0, 0.0, 0.0, 0.2, 0.2,
         0.4, 0.4, 0.6, 0.6, 0.8, 0.8, 1.0, 1.0,
         0.4, 0.4, 0.6, 0.6, 0.8, 0.8, 1.0, 1.0);
  // Try with cv::Size(8, 8).
  ImageData bigger_image_1 = image;
  bigger_image_1.ResizeImage(
      cv::Size(8, 8), super_resolution::INTERPOLATE_NEAREST);
  // Try with scale factor of 2.0.
  ImageData bigger_image_2 = image;
  bigger_image_2.ResizeImage(2.0, super_resolution::INTERPOLATE_NEAREST);
  // Check results.
  for (int channel_index = 0; channel_index < num_channels; ++channel_index) {
    EXPECT_TRUE(AreMatricesEqual(
        bigger_image_1.GetChannelImage(channel_index),
        expected_bigger_image));
    EXPECT_TRUE(AreMatricesEqual(
        bigger_image_2.GetChannelImage(channel_index),
        expected_bigger_image));
  }

  /* Verify that the additive interpolation implementation works. */

  // Upsampling with additive interpolation should pad the image with zeros.
  const cv::Mat expected_additive_upsampled = (cv::Mat_<double>(8, 8)
      << 0.1, 0.0, 0.2, 0.0, 0.3, 0.0, 0.4, 0.0,
         0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
         0.5, 0.0, 0.6, 0.0, 0.7, 0.0, 0.8, 0.0,
         0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
         0.9, 0.0, 1.0, 0.0, 0.0, 0.0, 0.2, 0.0,
         0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
         0.4, 0.0, 0.6, 0.0, 0.8, 0.0, 1.0, 0.0,
         0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  ImageData image_2(image_pixels, super_resolution::DO_NOT_NORMALIZE_IMAGE);
  image_2.ResizeImage(2, super_resolution::INTERPOLATE_ADDITIVE);
  EXPECT_TRUE(AreMatricesEqual(
      image_2.GetChannelImage(0),
      expected_additive_upsampled));

  // Downsampling with additive interpolation should add the values of the HR
  // patch in the downsampled pixels.
  const cv::Mat expected_additive_downsampled = (cv::Mat_<double>(2, 2)
      << (0.1 + 0.2 + 0.5 + 0.6), (0.3 + 0.4 + 0.7 + 0.8),
         (0.9 + 1.0 + 0.4 + 0.6), (0.0 + 0.2 + 0.8 + 1.0));
  ImageData image_3(image_pixels, super_resolution::DO_NOT_NORMALIZE_IMAGE);
  image_3.ResizeImage(0.5, super_resolution::INTERPOLATE_ADDITIVE);
  EXPECT_TRUE(AreMatricesEqual(
      image_3.GetChannelImage(0),
      expected_additive_downsampled));
}

// Tests the ChangeColorSpace method to see that the image is in fact being
// converted correctly.
TEST(ImageData, ChangeColorSpace) {
  // Build the input BGR image (4 x 4 x 3).
  cv::Mat input_image;
  cv::merge(kTestColorChannels, input_image);

  ImageData image(input_image, super_resolution::DO_NOT_NORMALIZE_IMAGE);
  EXPECT_EQ(image.GetNumChannels(), 3);

  // Get the expected conversion to the YCrCb color space.
  cv::Mat converted_image;
  input_image.convertTo(converted_image, CV_32F);
  cv::cvtColor(converted_image, converted_image, CV_BGR2YCrCb);
  converted_image.convertTo(converted_image, input_image.type());
  std::vector<cv::Mat> converted_channels;
  cv::split(converted_image, converted_channels);

  // Check that the image was converted correctly.
  image.ChangeColorSpace(super_resolution::SPECTRAL_MODE_COLOR_YCRCB);
  EXPECT_EQ(image.GetNumChannels(), 3);
  for (int channel_index = 0; channel_index < 3; ++channel_index) {
    EXPECT_TRUE(AreMatricesEqual(
        image.GetChannelImage(channel_index),
        converted_channels[channel_index],
        kPixelErrorTolerance));
  }

  // Verify that the visualization image is still BGR.
  cv::Mat visualization_image = image.GetVisualizationImage();
  visualization_image.convertTo(
      visualization_image, input_image.type(), 1.0 / 255.0);
  std::vector<cv::Mat> visualization_channels;
  cv::split(visualization_image, visualization_channels);
  EXPECT_EQ(visualization_channels.size(), 3);
  for (int channel_index = 0; channel_index < 3; ++channel_index) {
    EXPECT_TRUE(AreMatricesEqual(
        visualization_channels[channel_index],
        kTestColorChannels[channel_index],
        kPixelErrorTolerance));
  }

  // Verify that image operations also work on the converted image.
  ImageData image_resized = image;  // Copy to avoid corrupting the original.
  image_resized.ResizeImage(2, super_resolution::INTERPOLATE_NEAREST);
  EXPECT_EQ(image_resized.GetImageSize(), cv::Size(8, 8));
  cv::Mat converted_image_resized;
  cv::resize(
      converted_image,
      converted_image_resized,
      cv::Size(8, 8),
      0,  // (0, 0) for X, Y scale so it uses the given size instead.
      0,
      cv::INTER_NEAREST);
  std::vector<cv::Mat> converted_channels_resized;
  cv::split(converted_image_resized, converted_channels_resized);
  for (int channel_index = 0; channel_index < 3; ++channel_index) {
    EXPECT_TRUE(AreMatricesEqual(
        image_resized.GetChannelImage(channel_index),
        converted_channels_resized[channel_index],
        kPixelErrorTolerance));
  }

  // Now verify that the conversion back also works.
  image.ChangeColorSpace(super_resolution::SPECTRAL_MODE_COLOR_BGR);
  EXPECT_EQ(image.GetNumChannels(), 3);
  for (int channel_index = 0; channel_index < 3; ++channel_index) {
    EXPECT_TRUE(AreMatricesEqual(
        image.GetChannelImage(channel_index),
        kTestColorChannels[channel_index],
        kPixelErrorTolerance));
  }

  // Also verify that the resized image is converted back to resized BGR.
  image_resized.ChangeColorSpace(super_resolution::SPECTRAL_MODE_COLOR_BGR);
  cv::Mat input_image_resized;
  cv::resize(
      input_image,
      input_image_resized,
      cv::Size(8, 8),
      0,  // (0, 0) for X, Y scale so it uses the given size instead.
      0,
      cv::INTER_NEAREST);
  std::vector<cv::Mat> input_image_channels_resized;
  cv::split(input_image_resized, input_image_channels_resized);
  for (int channel_index = 0; channel_index < 3; ++channel_index) {
    EXPECT_TRUE(AreMatricesEqual(
        image_resized.GetChannelImage(channel_index),
        input_image_channels_resized[channel_index],
        kPixelErrorTolerance));
  }

  /* Now verify that the image works correctly in lumiance-only mode. */

  // Create a new test image to avoid snowballing numerical errors.
  ImageData image_2(input_image, super_resolution::DO_NOT_NORMALIZE_IMAGE);
  image_2.ChangeColorSpace(super_resolution::SPECTRAL_MODE_COLOR_YCRCB, true);
  EXPECT_EQ(image_2.GetNumChannels(), 1);
  EXPECT_TRUE(AreMatricesEqual(
      image_2.GetChannelImage(0), converted_channels[0], kPixelErrorTolerance));

  // Verify that resizing the image will work as before for the one channel.
  image_2.ResizeImage(2, super_resolution::INTERPOLATE_NEAREST);
  EXPECT_EQ(image_2.GetImageSize(), cv::Size(8, 8));
  EXPECT_TRUE(AreMatricesEqual(
      image_2.GetChannelImage(0),
      converted_channels_resized[0],
      kPixelErrorTolerance));

  // Verify that the visualization image is still 3-channel BGR. Since we
  // resized the image, it should automatically interpolate the color channels
  // with linear interpolation upsampling.
  cv::Mat visualization_image_2 = image_2.GetVisualizationImage();
  visualization_image_2.convertTo(
      visualization_image_2, input_image.type(), 1.0 / 255.0);
  std::vector<cv::Mat> visualization_channels_2;
  cv::split(visualization_image_2, visualization_channels_2);
  EXPECT_EQ(visualization_channels_2.size(), 3);
  for (int channel_index = 0; channel_index < 3; ++channel_index) {
    // Allow a much more forgiving error tolerance since the image was heavily
    // manipulated (converted => resized => converted back), and the colors
    // where interpolated with linear interpolation.
    EXPECT_TRUE(AreMatricesEqual(
        visualization_channels_2[channel_index],
        input_image_channels_resized[channel_index],
        0.15));
  }
}

TEST(ImageData, InterpolateColorFrom) {
  // Build the input BGR image (4 x 4 x 3).
  cv::Mat input_image;
  cv::merge(kTestColorChannels, input_image);

  // Get the expected conversion to the YCrCb color space.
  cv::Mat converted_image;
  input_image.convertTo(converted_image, CV_32F);
  cv::cvtColor(converted_image, converted_image, CV_BGR2YCrCb);
  converted_image.convertTo(converted_image, input_image.type());
  std::vector<cv::Mat> converted_channels;
  cv::split(converted_image, converted_channels);

  // Create the luminance-only monochrome image. Do not normalize. Copies
  // cv::Mat.
  ImageData luminance_image(
      converted_channels[0], super_resolution::DO_NOT_NORMALIZE_IMAGE);
  EXPECT_EQ(luminance_image.GetNumChannels(), 1);

  // Create the reference YCrCB image (converted from BGR image).
  ImageData reference_color_image(
      input_image, super_resolution::DO_NOT_NORMALIZE_IMAGE);
  reference_color_image.ChangeColorSpace(
      super_resolution::SPECTRAL_MODE_COLOR_YCRCB);
  EXPECT_EQ(reference_color_image.GetNumChannels(), 3);

  // Make a copy of the luminance image before changing it.
  ImageData luminance_image_2 = luminance_image;

  // Interpolate the colors and expect equal results.
  luminance_image.InterpolateColorFrom(reference_color_image);
  EXPECT_EQ(luminance_image.GetNumChannels(), 3);
  for (int channel_index = 0; channel_index < 3; ++channel_index) {
    EXPECT_TRUE(AreMatricesEqual(
        luminance_image.GetChannelImage(channel_index),
        converted_channels[channel_index],
        kPixelErrorTolerance));
  }

  /* Resize the image and verify that color interpolation still works. */

  // Resize the luminance image and converted "ground truth" image and make sure
  // that the first (luminance) channel matches.
  luminance_image_2.ResizeImage(2, super_resolution::INTERPOLATE_LINEAR);
  cv::Mat converted_image_resized;
  cv::resize(
      converted_image,
      converted_image_resized,
      cv::Size(8, 8),
      0,  // (0, 0) for X, Y scale so it uses the given size instead.
      0,
      cv::INTER_LINEAR);
  std::vector<cv::Mat> converted_channels_resized;
  cv::split(converted_image_resized, converted_channels_resized);
  EXPECT_TRUE(AreMatricesEqual(
      luminance_image_2.GetChannelImage(0),
      converted_channels_resized[0],
      kPixelErrorTolerance));

  // Now interplate the low-resolution color data into the luminance image and
  // make sure we still get color interpolation which will use linear
  // interpolation to scale up the reference color channels.
  EXPECT_NE(
      luminance_image_2.GetImageSize(), reference_color_image.GetImageSize());
  luminance_image_2.InterpolateColorFrom(reference_color_image);
  EXPECT_EQ(luminance_image_2.GetNumChannels(), 3);
  for (int channel_index = 0; channel_index < 3; ++channel_index) {
    EXPECT_TRUE(AreMatricesEqual(
        luminance_image_2.GetChannelImage(channel_index),
        converted_channels_resized[channel_index],
        kPixelErrorTolerance));
  }
}

// Tests the multiplication and addition methods for the ImageData object,
// including the overloaded operators.
TEST(ImageData, AddMultiplyDivideImage) {
  cv::Mat image_matrix;
  cv::merge(kTestColorChannels, image_matrix);
  ImageData image(image_matrix, super_resolution::DO_NOT_NORMALIZE_IMAGE);

  ImageData test_image_1 = image;  // Copy.
  test_image_1.MultiplyByScalar(3.0);
  EXPECT_EQ(test_image_1.GetNumChannels(), 3);
  EXPECT_DOUBLE_EQ(test_image_1.GetPixelValue(0, 0), 0.3);  // Was 0.1.
  EXPECT_DOUBLE_EQ(test_image_1.GetPixelValue(1, 1), 0.9);  // Was 0.3.
  EXPECT_DOUBLE_EQ(test_image_1.GetPixelValue(2, 2), 0.3);  // Was 0.1.

  const ImageData test_image_2 = image * -2.0;
  EXPECT_EQ(test_image_2.GetNumChannels(), 3);
  EXPECT_DOUBLE_EQ(test_image_2.GetPixelValue(0, 4), -0.3);  // Was 0.15.
  EXPECT_DOUBLE_EQ(test_image_2.GetPixelValue(1, 4), -0.2);  // Was 0.1.
  EXPECT_DOUBLE_EQ(test_image_2.GetPixelValue(2, 15), -0.3);  // Was 0.15.

  const ImageData test_image_3 = image / 2.0;
  EXPECT_EQ(test_image_3.GetNumChannels(), 3);
  EXPECT_DOUBLE_EQ(test_image_3.GetPixelValue(0, 5), 0.125);  // Was 0.25.
  EXPECT_DOUBLE_EQ(test_image_3.GetPixelValue(1, 7), 0.2);  // Was 0.4.
  EXPECT_DOUBLE_EQ(test_image_3.GetPixelValue(2, 4), 0.0);  // Was 0.0.

  const ImageData test_image_4 = test_image_1 + test_image_3;
  EXPECT_EQ(test_image_4.GetNumChannels(), 3);
  EXPECT_DOUBLE_EQ(test_image_4.GetPixelValue(0, 0), 0.35);  // 0.3 + 0.05.
  EXPECT_DOUBLE_EQ(test_image_4.GetPixelValue(1, 1), 1.05);  // 0.9 + 0.15.
  EXPECT_DOUBLE_EQ(test_image_4.GetPixelValue(2, 2), 0.35);  // 0.3 + 0.05.
}

// Tests that the report for analyzing images is correctly generated.
TEST(ImageData, GetImageDataReport) {
  const double pixel_values[(5 * 3) * 2] = {
      // Channel 1:
      -0.1,  0.2,  0.3,  0.4,  -0.5,
      0.15, 0.25, -1.35, 0.45, 0.55,
       0.6,  1.65, 0.7,  0.75, 1.8,
      // Channel 2:
       0.6,  1.5,  0.33,  0.1,  0.2,
      1.82, 0.15, 0.35, 3.54,  0.5,
       1.6,  0.62, 1.0,  9.23, -9.9
  };
  ImageData image(pixel_values, cv::Size(5, 3), 2);
  super_resolution::ImageDataReport report = image.GetImageDataReport();
  EXPECT_EQ(report.image_size, cv::Size(5, 3));
  EXPECT_EQ(report.num_channels, 2);
  EXPECT_EQ(report.num_negative_pixels, 4);
  EXPECT_EQ(report.num_over_one_pixels, 7);
  EXPECT_EQ(report.channel_with_most_negative_pixels, 0);
  EXPECT_EQ(report.max_num_negative_pixels_in_one_channel, 3);
  EXPECT_EQ(report.channel_with_most_over_one_pixels, 1);
  EXPECT_EQ(report.max_num_over_one_pixels_in_one_channel, 5);
  EXPECT_EQ(report.smallest_pixel_value, -9.9);
  EXPECT_EQ(report.largest_pixel_value, 9.23);
}

// This test verifies that the correct visualization image is returned for
// different numbers of channels.
TEST(ImageData, GetVisualizationImage) {
  // TODO: implement.
  // cv::Mat GetVisualizationImage
}
