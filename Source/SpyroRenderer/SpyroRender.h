#pragma once

#ifdef SPYRORENDER
namespace SpyroRender {
	void Open();
	void Close();

	void OnLevelEntry();

	void OnUpdateLace();

	void UpdateTextures(bool doOverwriteExistingTextures = false);
};
#endif