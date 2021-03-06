#pragma once

#include "engine/core/render/interface/Renderable.h"

namespace Echo
{
	class VKRenderable : public Renderable
	{
	public:
		VKRenderable(const String& renderStage, ShaderProgram* shader, int identifier);
        virtual ~VKRenderable() {}

        // link shader and program
        virtual void link() override {}
	};
}
