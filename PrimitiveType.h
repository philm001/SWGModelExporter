#pragma once


enum ShaderPrimitiveType
{
	SPSPT_pointList,
	SPSPT_lineList,
	SPSPT_lineStrip,
	SPSPT_triangleList,
	SPSPT_triangleStrip,
	SPSPT_triangleFan,

	SPSPT_indexedPointList,
	SPSPT_indexedLineList,
	SPSPT_indexedLineStrip,
	SPSPT_indexedTriangleList,
	SPSPT_indexedTriangleStrip,
	SPSPT_indexedTriangleFan,

	SPSPT_max
};