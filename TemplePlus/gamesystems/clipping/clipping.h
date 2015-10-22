#pragma once

class Graphics;
class BinaryReader;

class MapClipping {
public:
	explicit MapClipping(Graphics& g);
	~MapClipping();

	void Load(const std::string& directory);
	void Render();
	void Unload();

	MapClipping(MapClipping&) = delete;
	MapClipping(MapClipping&&) = delete;
	MapClipping& operator=(MapClipping&) = delete;
	MapClipping& operator=(MapClipping&&) = delete;

private:
	class Impl;

	void LoadMeshes(const std::string &directory);
	void LoadObjects(const std::string &directory);
	void LoadObject(BinaryReader& reader);

	std::unique_ptr<Impl> mImpl;		
};
