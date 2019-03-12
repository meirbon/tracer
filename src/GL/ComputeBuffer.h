#pragma once

#include <GL/glew.h>

class ComputeBuffer
{
  public:
    ComputeBuffer(unsigned int nrOfBytes, void *hostData = nullptr);
    ~ComputeBuffer();

    inline unsigned int *GetHostPointer()
    {
        return (unsigned int *)glMapBuffer(GL_SHADER_STORAGE_BUFFER,
                                           GL_READ_WRITE);
    }

    inline void Unmap() { glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); }

    inline const unsigned int &GetID() const { return m_Id; }

    inline void Bind(unsigned int index)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_Id);
    }

    inline void Unbind(unsigned int index)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, 0);
    }

  private:
    unsigned int m_Id;
    unsigned int m_Size;
    unsigned int *m_HostBuffer;
};