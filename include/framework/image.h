#pragma once

#include <vector>
#include <cstddef>
#include <cassert>
#include <cstring>

namespace framework
{
	struct Image
	{
		template<typename Pixel>
		static Image make(std::uint32_t width, std::uint32_t height, std::uint32_t channel_count, const void* data = nullptr);

		static Image make(std::uint32_t width, std::uint32_t height, std::uint32_t pixel_sizeof, std::uint32_t channel_count, const void* data = nullptr);

		template<typename T>
		T& at(std::uint32_t x, std::uint32_t y);

		template<typename T>
		const T& at(std::uint32_t x, std::uint32_t y) const;

		template<typename T>
		Image operator*(const T rhs) const;

		template<typename T>
		Image add(const Image& rhs) const;

		template<typename T>
		Image mul(const Image& rhs) const;

		Image extract_roi(std::uint32_t x, std::uint32_t y, std::uint32_t w, std::uint32_t h) const;

		std::uint32_t m_width = {};
		std::uint32_t m_height = {};
		std::uint32_t m_channel_count = {};
		std::uint32_t m_pixel_sizeof = {};
		std::vector<std::byte> m_pixels;
	};

	template<typename T>
	Image operator-(const T rhs, const Image& lhs);

	template<typename Pixel>
	inline Image Image::make(std::uint32_t width, std::uint32_t height, std::uint32_t channel_count, const void* data)
	{
		return Image::make(width, height, sizeof(Pixel), channel_count, data);
	}

	inline Image Image::make(std::uint32_t width, std::uint32_t height, std::uint32_t pixel_sizeof, std::uint32_t channel_count, const void* data)
	{
		assert(pixel_sizeof % channel_count == 0);
		Image dst;
		dst.m_width = width;
		dst.m_height = height;
		dst.m_channel_count = channel_count;
		dst.m_pixel_sizeof = pixel_sizeof;
		dst.m_pixels.resize(pixel_sizeof * width * height);
		if (data)
			std::memcpy(dst.m_pixels.data(), data, dst.m_pixels.size());
		return dst;
	}

	template<typename T>
	inline Image Image::add(const Image& rhs) const
	{
		assert(m_pixel_sizeof == rhs.m_pixel_sizeof);
		Image dst = Image::make<T>(m_width, m_height, m_channel_count);
		for (std::uint32_t y = 0; y < m_height; ++y)
			for (std::uint32_t x = 0; x < m_width; ++x)
				dst.at<T>(x, y) = at<T>(x, y) + rhs.at<T>(x, y);

		return dst;
	}

	template<typename T>
	inline Image Image::mul(const Image& rhs) const
	{
		assert(m_pixel_size == rhs.m_pixel_size);
		Image dst = Image::make<T>(m_width, m_height, m_channel_count);
		for (std::uint32_t y = 0; y < m_height; ++y)
			for (std::uint32_t x = 0; x < m_width; ++x)
				dst.at<T>(x, y) = at<T>(x, y) * rhs.at<T>(x, y);

		return dst;
	}

	template<typename T>
	inline T& Image::at(std::uint32_t x, std::uint32_t y)
	{
		return *reinterpret_cast<T*>(m_pixels.data() + (x + y * m_width) * m_pixel_sizeof);
	}

	template<typename T>
	inline const T& Image::at(std::uint32_t x, std::uint32_t y) const
	{
		return *reinterpret_cast<const T*>(m_pixels.data() + (x + y * m_width) * m_pixel_sizeof);
	}

	template<typename T>
	inline Image Image::operator*(const T rhs) const
	{
		assert(sizeof(T) == m_pixel_sizeof);
		Image dst = Image::make<T>(m_width, m_height, m_channel_count);
		for (std::uint32_t y = 0; y < m_height; ++y)
			for (std::uint32_t x = 0; x < m_width; ++x)
				dst.at<T>(x, y) = at<T>(x, y) * rhs;

		return dst;
	}

	template<typename T>
	Image operator-(const T rhs, const Image& lhs)
	{
		assert(sizeof(T) == lhs.m_pixel_sizeof);
		Image dst = Image::make<T>(lhs.m_width, lhs.m_height, lhs.m_channel_count);
		for (std::uint32_t y = 0; y < lhs.m_height; ++y)
			for (std::uint32_t x = 0; x < lhs.m_width; ++x)
				dst.at<T>(x, y) = rhs - lhs.at<T>(x, y);

		return dst;
	}

	inline Image Image::extract_roi(std::uint32_t x, std::uint32_t y, std::uint32_t w, std::uint32_t h) const
	{
		assert((x + w) <= m_width && (y + h) <= m_height);
		Image dst = Image::make(w, h, m_pixel_sizeof, m_channel_count);
		for (std::uint32_t row = 0; row < h; ++row)
			std::memcpy(&dst.at<std::byte>(0, row), &at<std::byte>(x, y + row), m_pixel_sizeof * w);
		return dst;
	}
}