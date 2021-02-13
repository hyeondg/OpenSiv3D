﻿//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (c) 2008-2021 Ryo Suzuki
//	Copyright (c) 2016-2021 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

# include <Siv3D/FileSystem.hpp>
# include <Siv3D/Font/IFont.hpp>
# include <Siv3D/Common/Siv3DEngine.hpp>
# include "FontData.hpp"
# include "GlyphRenderer/GlyphRenderer.hpp"
# include "GlyphRenderer/BitmapGlyphRenderer.hpp"
# include "GlyphRenderer/OutlineGlyphRenderer.hpp"
# include "GlyphRenderer/SDFGlyphRenderer.hpp"
# include "GlyphRenderer/MSDFGlyphRenderer.hpp"
# include "GlyphCache/BitmapGlyphCache.hpp"
# include "GlyphCache/SDFGlyphCache.hpp"
# include "GlyphCache/MSDFGlyphCache.hpp"

namespace s3d
{
	FontData::FontData(Null)
	{
		m_glyphCache = std::make_unique<BitmapGlyphCache>();

		m_initialized = true;
	}

	FontData::FontData(const FT_Library library, const FilePathView path, const size_t faceIndex, const FontMethod fontMethod, const int32 fontSize, const FontStyle style)
	{
	# if SIV3D_PLATFORM(WINDOWS)

		if (FileSystem::IsResource(path))
		{
			m_resource = FontResourceHolder{ path };

			if (not m_fontFace.load(library, m_resource.data(), m_resource.size(), faceIndex, fontSize, style))
			{
				return;
			}
		}
		else
		{
			if (not m_fontFace.load(library, path, faceIndex, fontSize, style))
			{
				return;
			}
		}

	# else

		if (not m_fontFace.load(library, path, faceIndex, fontSize, style))
		{
			return;
		}

	# endif

		switch (fontMethod)
		{
		case FontMethod::Bitmap:
			m_glyphCache = std::make_unique<BitmapGlyphCache>();
			break;
		case FontMethod::SDF:
			m_glyphCache = std::make_unique<SDFGlyphCache>();
			break;
		case FontMethod::MSDF:
			m_glyphCache = std::make_unique<MSDFGlyphCache>();
			break;
		}

		m_method = fontMethod;

		m_initialized = true;
	}

	FontData::~FontData()
	{

	}

	bool FontData::isInitialized() const noexcept
	{
		return m_initialized;
	}

	const FontFaceProperty& FontData::getProperty() const noexcept
	{
		return m_fontFace.getProperty();
	}

	FontMethod FontData::getMethod() const
	{
		return m_method;
	}

	bool FontData::hasGlyph(const StringView ch)
	{
		const HBGlyphInfo glyphInfo = m_fontFace.getHBGlyphInfo(ch);

		if (glyphInfo.count != 1)
		{
			return false;
		}
		
		const GlyphIndex glyphIndex = glyphInfo.info[0].codepoint;

		return (glyphIndex != 0);
	}

	GlyphIndex FontData::getGlyphIndex(const StringView ch)
	{
		const HBGlyphInfo glyphInfo = m_fontFace.getHBGlyphInfo(ch);

		if (glyphInfo.count != 1)
		{
			return 0;
		}

		const GlyphIndex glyphIndex = glyphInfo.info[0].codepoint;

		return glyphIndex;
	}

	Array<GlyphCluster> FontData::getGlyphClusters(const StringView s, bool recursive) const
	{
		const HBGlyphInfo glyphInfo = m_fontFace.getHBGlyphInfo(s);

		const size_t count = glyphInfo.count;

		if (recursive && m_fallbackFonts)
		{
			Array<GlyphCluster> result;

			for (size_t i = 0; i < count;)
			{
				const GlyphCluster clusterA{ glyphInfo.info[i].codepoint, 0, glyphInfo.info[i].cluster };

				if (clusterA.glyphIndex != 0)
				{
					result << clusterA;
					++i;
					continue;
				}

				const size_t pos = clusterA.pos;
				size_t k = 0;

				for (;;)
				{
					++k;

					if ((count <= (i + k))
						|| (glyphInfo.info[(i + k)].cluster != pos))
					{
						break;
					}
				}

				bool fallbackDone = false;
				uint32 fallbackIndex = 1;

				for (const auto& fallbackFont : m_fallbackFonts)
				{
					if (fallbackFont.expired())
					{
						continue;
					}

					const Array<GlyphCluster> clustersB = 
						SIV3D_ENGINE(Font)->getGlyphClusters(fallbackFont.lock()->id(), s.substr(i, k), false);

					if (clustersB.none([](const GlyphCluster& g) { return (g.glyphIndex == 0); }))
					{
						for (size_t m = 0; m < clustersB.size(); ++m)
						{
							const auto& clusterB = clustersB[m];
							result << GlyphCluster{ clusterB.glyphIndex, fallbackIndex, pos };
						}

						fallbackDone = true;
						break;
					}

					++fallbackIndex;
				}

				if (not fallbackDone)
				{
					for (size_t m = 0; m < k; ++m)
					{
						result << GlyphCluster{ glyphInfo.info[(i + m)].codepoint, 0, glyphInfo.info[(i + m)].cluster };
					}
				}

				i += k;
			}

			return result;
		}
		else
		{
			Array<GlyphCluster> clusters(count);

			for (size_t i = 0; i < count; ++i)
			{
				clusters[i] = { glyphInfo.info[i].codepoint, 0, glyphInfo.info[i].cluster };
			}

			return clusters;
		}
	}

	GlyphInfo FontData::getGlyphInfoByGlyphIndex(const GlyphIndex glyphIndex) const
	{
		return GetGlyphInfo(m_fontFace.getFT_Face(), glyphIndex, m_fontFace.getProperty());
	}

	OutlineGlyph FontData::renderOutlineByGlyphIndex(const GlyphIndex glyphIndex, const CloseRing closeRing) const
	{
		return RenderOutlineGlyph(m_fontFace.getFT_Face(), glyphIndex, closeRing, m_fontFace.getProperty());
	}

	Array<OutlineGlyph> FontData::renderOutlines(const StringView s, const CloseRing closeRing) const
	{
		const HBGlyphInfo glyphInfo = m_fontFace.getHBGlyphInfo(s);

		Array<OutlineGlyph> results(Arg::reserve = glyphInfo.count);

		for (size_t i = 0; i < glyphInfo.count; ++i)
		{
			const GlyphIndex glyphIndex = glyphInfo.info[i].codepoint;

			results << RenderOutlineGlyph(m_fontFace.getFT_Face(), glyphIndex, closeRing, m_fontFace.getProperty());
		}

		return results;
	}

	BitmapGlyph FontData::renderBitmapByGlyphIndex(const GlyphIndex glyphIndex) const
	{
		return RenderBitmapGlyph(m_fontFace.getFT_Face(), glyphIndex, m_fontFace.getProperty());
	}

	SDFGlyph FontData::renderSDFByGlyphIndex(const GlyphIndex glyphIndex, const int32 buffer) const
	{
		return RenderSDFGlyph(m_fontFace.getFT_Face(), glyphIndex, buffer, m_fontFace.getProperty());
	}

	MSDFGlyph FontData::renderMSDFByGlyphIndex(const GlyphIndex glyphIndex, const int32 buffer) const
	{
		return RenderMSDFGlyph(m_fontFace.getFT_Face(), glyphIndex, buffer, m_fontFace.getProperty());
	}

	IGlyphCache& FontData::getGlyphCache() const
	{
		return *m_glyphCache;
	}

	bool FontData::addFallbackFont(const std::weak_ptr<AssetHandle<Font>::AssetIDWrapperType>& font)
	{
		m_fallbackFonts.push_back(font);

		return true;
	}

	const std::weak_ptr<AssetHandle<Font>::AssetIDWrapperType>& FontData::getFallbackFont(const size_t index) const
	{
		return m_fallbackFonts[index];
	}
}
