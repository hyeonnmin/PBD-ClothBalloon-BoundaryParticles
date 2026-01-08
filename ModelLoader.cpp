#include "ModelLoader.h"

#include <filesystem>

namespace jhm {

    using namespace DirectX::SimpleMath;

    void ModelLoader::Load(std::string basePath, std::string filename) {
        this->basePath = basePath;

        Assimp::Importer importer;

        importer.SetPropertyInteger(
            AI_CONFIG_PP_RVC_FLAGS,
            aiComponent_NORMALS |
            aiComponent_TANGENTS_AND_BITANGENTS |
            aiComponent_TEXCOORDS
        );

        const aiScene* pScene = importer.ReadFile(
            this->basePath + filename,
            aiProcess_RemoveComponent | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded);

        Matrix tr; // Initial transformation
        ProcessNode(pScene->mRootNode, pScene, tr);

    }

    void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, Matrix tr) {
        
        Matrix m;
        ai_real* temp = &node->mTransformation.a1;
        float* mTemp = &m._11;
        for (int t = 0; t < 16; t++) {
            mTemp[t] = float(temp[t]);
        }
        m = m.Transpose() * tr;

        for (UINT i = 0; i < node->mNumMeshes; i++) {


            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            auto newMesh = this->ProcessMesh(mesh, scene);

            for (auto& v : newMesh.vertices) {
                v.position = DirectX::SimpleMath::Vector3::Transform(v.position, m);
            }

            meshes.push_back(newMesh);
        }

        for (UINT i = 0; i < node->mNumChildren; i++) {
            this->ProcessNode(node->mChildren[i], scene, m);
        }
    }

    MeshData ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene) {
        // Data to fill
        std::vector<Vertex> vertices;
        std::vector<Vertex2> vertices2;
        std::vector<uint32_t> indices;

        // Walk through each of the mesh's vertices
        for (UINT i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            Vertex2 vertex2;

            vertex.position.x = mesh->mVertices[i].x;
            vertex.position.y = mesh->mVertices[i].y;
            vertex.position.z = mesh->mVertices[i].z;

            vertex2.velocity.x = 0.0f;
            vertex2.velocity.y = 0.0f;
            vertex2.velocity.z = 0.0f;

            vertex2.invMass = 1.0f;
            

            if (mesh->mTextureCoords[0]) {
                vertex.texcoord.x = (float)mesh->mTextureCoords[0][i].x;
                vertex.texcoord.y = (float)mesh->mTextureCoords[0][i].y;
            }

            vertices.push_back(vertex);
            vertices2.push_back(vertex2);
        }


        for (UINT i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
  
            for (UINT j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }

        }

        std::cout << "vertices size : " << vertices.size() << std::endl;
        std::cout << "indices size : " << indices.size() << std::endl;

        MeshData newMesh;
        newMesh.vertices = vertices;
        newMesh.vertices2 = vertices2;
        newMesh.indices = indices;


        return newMesh;
    }

}