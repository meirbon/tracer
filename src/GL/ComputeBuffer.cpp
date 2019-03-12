#include "ComputeBuffer.h"

ComputeBuffer::ComputeBuffer(unsigned int nrOfBytes, void *hostData)
{
    m_Size = nrOfBytes;
    glGenBuffers(1, &m_Id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_Id);

    if (hostData != nullptr)
    {
        glBufferData(GL_SHADER_STORAGE_BUFFER, nrOfBytes, hostData,
                     GL_STATIC_DRAW);
    }
    else
    {
        glBufferData(GL_SHADER_STORAGE_BUFFER, nrOfBytes, nullptr,
                     GL_STATIC_DRAW);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
}

ComputeBuffer::~ComputeBuffer() { glDeleteBuffers(1, &m_Id); }