//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (c) 2008-2019 Ryo Suzuki
//	Copyright (c) 2016-2019 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

# pragma once
# include <Siv3D/ByteArray.hpp>
# include <Siv3D/Optional.hpp>
# include <Siv3D/SamplerState.hpp>
# include <GL/glew.h>
# include <GLFW/glfw3.h>

namespace s3d
{
	class PixelShader_GL
	{
	private:

		GLuint m_psProgram = 0;
		
		Array<std::pair<uint32, GLint>> m_textureIndices;
		
		bool m_initialized = false;
		
	public:

		struct Null {};
		
		PixelShader_GL() = default;
		
		PixelShader_GL(Null)
		{
			m_initialized = true;
		}
		
		~PixelShader_GL()
		{
			if (m_psProgram)
			{
				::glDeleteProgram(m_psProgram);
			}
		}
		
		PixelShader_GL(const String& source)
		{
			const std::string sourceUTF8 = source.toUTF8();
			
			const char* pSource = sourceUTF8.c_str();
			
			m_psProgram = ::glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &pSource);
			
			GLint status = GL_FALSE;
			
			::glGetProgramiv(m_psProgram, GL_LINK_STATUS, &status);
			
			GLint logLen = 0;
			
			::glGetProgramiv(m_psProgram, GL_INFO_LOG_LENGTH, &logLen);
			
			if (logLen > 4)
			{
				std::string log(logLen + 1, '\0');
				
				::glGetProgramInfoLog(m_psProgram, logLen, &logLen, &log[0]);
				
				LOG_FAIL(U"❌ Pixel shader compilation failed: {0}"_fmt(Unicode::Widen(log)));
			}
			
			if (status == GL_FALSE)
			{
				::glDeleteProgram(m_psProgram);
				
				m_psProgram = 0;
			}
			
			if (m_psProgram)
			{
				for (uint32 slot = 0; slot < SamplerState::MaxSamplerCount; ++slot)
				{
					const std::string name = Format(U"Texture", slot).narrow();
					
					const GLint location = ::glGetUniformLocation(m_psProgram, name.c_str());
					
					if (location != -1)
					{
						m_textureIndices.emplace_back(slot, location);
					}
				}
			}
			
			m_initialized = m_psProgram != 0;
		}
		
		bool isInitialized() const noexcept
		{
			return m_initialized;
		}
		
		GLint getProgram() const
		{
			return m_psProgram;
		}
		
		void setPSSamplerUniform()
		{
			if (m_textureIndices)
			{
				::glUseProgram(m_psProgram);
				
				for (auto[slot, location] : m_textureIndices)
				{
					::glUniform1i(location, slot);
				}
				
				::glUseProgram(0);
			}
		}
		
		GLuint getUniformBlockIndex(const char* const name)
		{
			return ::glGetUniformBlockIndex(m_psProgram, name);
		}
		
		void setUniformBlockBinding(const char* const name, GLuint index)
		{
			::glUniformBlockBinding(m_psProgram, getUniformBlockIndex(name), index);
		}
	};
}
