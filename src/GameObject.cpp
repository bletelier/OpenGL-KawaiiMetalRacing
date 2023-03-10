#include "GameObject.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GameObject::GameObject(){}

GameObject::GameObject(const char* path, GLuint shaderprog, btScalar masa, btVector3 startPosition,
         btQuaternion startRotation,btDiscreteDynamicsWorld* dynamicsWorld,
         const char* texture_path)
{
    this->world= dynamicsWorld;
    this->mass = masa;
    this->position = startPosition;
    this->rotation = startRotation;


    btCollisionShape* coll;
    if(load_mesh(path, vao, this->vertNumber,&coll)==false){
        printf("Error loading %s", path);
    }

    //TEXTURES
    load_texture2(shader_programme,texture_path,texture,tex_location);
    btTransform startTransform;
    startTransform.setIdentity();

    bool isDynamic = (this->mass != 0.f);
    btVector3 localInertia(0, 0, 0);
    if (isDynamic)
        coll->calculateLocalInertia(this->mass, localInertia);

    startTransform.setOrigin(this->position);
    startTransform.setRotation(this->rotation);

    btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(this->mass, myMotionState, coll, localInertia);
    this->rigidBody = new btRigidBody(rbInfo);
    dynamicsWorld->addRigidBody(this->rigidBody);

}

//NO_TEXTURE
GameObject::GameObject(const char* path, GLuint shaderprog, btScalar masa, btVector3 startPosition,
         btQuaternion startRotation,btDiscreteDynamicsWorld* dynamicsWorld)
{
    this->world= dynamicsWorld;
    this->mass = masa;
    this->position = startPosition;
    this->rotation = startRotation;
    this->shader_programme = shader_programme;
    btCollisionShape* coll;
    if(load_mesh(path, vao, this->vertNumber,&coll)==false){
        printf("Error loading %s", path);
    }
    btTransform startTransform;
    startTransform.setIdentity();

    bool isDynamic = (this->mass != 0.f);
    btVector3 localInertia(0, 0, 0);
    if (isDynamic)
        coll->calculateLocalInertia(this->mass, localInertia);

    startTransform.setOrigin(this->position);
    startTransform.setRotation(this->rotation);

    btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(this->mass, myMotionState, coll, localInertia);
    this->rigidBody = new btRigidBody(rbInfo);
    dynamicsWorld->addRigidBody(this->rigidBody);
}

GameObject::~GameObject(){
    delete this->rigidBody->getMotionState();
    delete this->rigidBody->getCollisionShape();
    delete this->rigidBody;
}

bool GameObject::load_mesh (const char* file_name, GLuint& vao, int& vert_no, btCollisionShape** col) {
    const aiScene* scene = aiImportFile (file_name, aiProcess_Triangulate);
    if (!scene) {
        fprintf (stderr, "ERROR: reading mesh %s\n", file_name);
        return false;
    }
    printf ("  %i animations\n", scene->mNumAnimations);
    printf ("  %i cameras\n", scene->mNumCameras);
    printf ("  %i lights\n", scene->mNumLights);
    printf ("  %i materials\n", scene->mNumMaterials);
    printf ("  %i meshes\n", scene->mNumMeshes);
    printf ("  %i textures\n", scene->mNumTextures);
    
    /* get first mesh in file only */
    const aiMesh* mesh = scene->mMeshes[0];
    printf ("    %i vertices in %s\n", mesh->mNumVertices, file_name);
    
    /* pass back number of vertex points in mesh */
    vert_no = mesh->mNumVertices;
    
    /* generate a VAO, using the pass-by-reference parameter that we give to the
    function */
    glGenVertexArrays (1, &vao);
    glBindVertexArray (vao);
    
    /* we really need to copy out all the data from AssImp's funny little data
    structures into pure contiguous arrays before we copy it into data buffers
    because assimp's texture coordinates are not really contiguous in memory.
    i allocate some dynamic memory to do this. */
    GLfloat* points = NULL;                                 // array of vertex points
    GLfloat* normals = NULL;                                // array of vertex normals
    GLfloat* texcoords = NULL;                              // array of texture coordinates
    btConvexHullShape* collider = new btConvexHullShape();  // mesh collider shape

    if (mesh->HasPositions ()) {
        points = (GLfloat*)malloc (vert_no * 3 * sizeof (GLfloat));
        for (int i = 0; i < vert_no; i++) {
            const aiVector3D* vp = &(mesh->mVertices[i]);
            points[i * 3] = (GLfloat)vp->x;
            points[i * 3 + 1] = (GLfloat)vp->y;
            points[i * 3 + 2] = (GLfloat)vp->z;

            //Se agregan todos los vertices del mesh al collisionObject 1 por 1
            collider->addPoint(btVector3(vp->x, vp->y, vp->z));
        }
    }
    if (mesh->HasNormals ()) {
        normals = (GLfloat*)malloc (vert_no * 3 * sizeof (GLfloat));
        for (int i = 0; i < vert_no; i++) {
            const aiVector3D* vn = &(mesh->mNormals[i]);
            normals[i * 3] = (GLfloat)vn->x;
            normals[i * 3 + 1] = (GLfloat)vn->y;
            normals[i * 3 + 2] = (GLfloat)vn->z;
        }
    }
    if (mesh->HasTextureCoords (0)) {
        printf("    %i texture coords\n", vert_no*2);
        texcoords = (GLfloat*)malloc (vert_no * 2 * sizeof (GLfloat));
        for (int i = 0; i < vert_no; i++) {
            const aiVector3D* vt = &(mesh->mTextureCoords[0][i]);
            texcoords[i * 2] = (GLfloat)vt->x;
            texcoords[i * 2 + 1] = (GLfloat)vt->y;
        }
    }
    printf("    %i vertices in collider non optimized\n", collider->getNumPoints());

    //C??digo optimizaci??n basado en http://www.bulletphysics.org/mediawiki-1.5.8/index.php/BtShapeHull_vertex_reduction_utility
    //Ligeramente modificado, ya que lo que sale en la wiki esta malo
    btShapeHull* hull = new btShapeHull(collider);
    btScalar margin = collider->getMargin();
    hull->buildHull(margin);
    btConvexHullShape* simplifiedConvexShape = new btConvexHullShape((const btScalar*)hull->getVertexPointer(), hull->numVertices(), sizeof(btVector3));
    printf("    %i vertices in collider optimized\n", simplifiedConvexShape->getNumPoints());

    //libera memoria del primer collider no-optimzado
    delete collider;

    *col = simplifiedConvexShape;

    /* copy mesh data into VBOs */
    if (mesh->HasPositions ()) {
        GLuint vbo;
        glGenBuffers (1, &vbo);
        glBindBuffer (GL_ARRAY_BUFFER, vbo);
        glBufferData (
            GL_ARRAY_BUFFER,
            3 * vert_no * sizeof (GLfloat),
            points,
            GL_STATIC_DRAW
        );
        glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray (0);
        free (points);
    }
    if (mesh->HasNormals ()) {
        GLuint vbo;
        glGenBuffers (1, &vbo);
        glBindBuffer (GL_ARRAY_BUFFER, vbo);
        glBufferData (
            GL_ARRAY_BUFFER,
            3 * vert_no * sizeof (GLfloat),
            normals,
            GL_STATIC_DRAW
        );
        glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray (1);
        free (normals);
    }
    if (mesh->HasTextureCoords (0)) {
        GLuint vbo;
        glGenBuffers (1, &vbo);
        glBindBuffer (GL_ARRAY_BUFFER, vbo);
        glBufferData (
            GL_ARRAY_BUFFER,
            2 * vert_no * sizeof (GLfloat),
            texcoords,
            GL_STATIC_DRAW
        );
        glVertexAttribPointer (2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray (2);
        free (texcoords);
    }
    if (mesh->HasTangentsAndBitangents ()) {
        // NB: could store/print tangents here
    }
    
    aiReleaseImport (scene);
    printf ("mesh loaded\n");
    
    return true;
}

bool GameObject::load_mesh (const char* file_name, GLuint& vao, int& vert_no) {
    const aiScene* scene = aiImportFile (file_name, aiProcess_Triangulate);
    if (!scene) {
        fprintf (stderr, "ERROR: reading mesh %s\n", file_name);
        return false;
    }
    printf ("  %i animations\n", scene->mNumAnimations);
    printf ("  %i cameras\n", scene->mNumCameras);
    printf ("  %i lights\n", scene->mNumLights);
    printf ("  %i materials\n", scene->mNumMaterials);
    printf ("  %i meshes\n", scene->mNumMeshes);
    printf ("  %i textures\n", scene->mNumTextures);

    const aiMesh* mesh = scene->mMeshes[0];
    printf ("    %i vertices in %s\n", mesh->mNumVertices, file_name);

    vert_no = mesh->mNumVertices;

    glGenVertexArrays (1, &vao);
    glBindVertexArray (vao);

    GLfloat* points = NULL;
    GLfloat* normals = NULL;
    GLfloat* texcoords = NULL;

    if (mesh->HasPositions ()) {
        points = (GLfloat*)malloc (vert_no * 3 * sizeof (GLfloat));
        for (int i = 0; i < vert_no; i++) {
            const aiVector3D* vp = &(mesh->mVertices[i]);
            points[i * 3] = (GLfloat)vp->x;
            points[i * 3 + 1] = (GLfloat)vp->y;
            points[i * 3 + 2] = (GLfloat)vp->z;
        }
    }
    if (mesh->HasNormals ()) {
        normals = (GLfloat*)malloc (vert_no * 3 * sizeof (GLfloat));
        for (int i = 0; i < vert_no; i++) {
            const aiVector3D* vn = &(mesh->mNormals[i]);
            normals[i * 3] = (GLfloat)vn->x;
            normals[i * 3 + 1] = (GLfloat)vn->y;
            normals[i * 3 + 2] = (GLfloat)vn->z;
        }
    }
    if (mesh->HasTextureCoords (0)) {
        texcoords = (GLfloat*)malloc (vert_no * 2 * sizeof (GLfloat));
        for (int i = 0; i < vert_no; i++) {
            const aiVector3D* vt = &(mesh->mTextureCoords[0][i]);
            texcoords[i * 2] = (GLfloat)vt->x;
            texcoords[i * 2 + 1] = (GLfloat)vt->y;
        }
    }

    if (mesh->HasPositions ()) {
        GLuint vbo;
        glGenBuffers (1, &vbo);
        glBindBuffer (GL_ARRAY_BUFFER, vbo);
        glBufferData (
            GL_ARRAY_BUFFER,
            3 * vert_no * sizeof (GLfloat),
            points,
            GL_STATIC_DRAW
        );
        glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray (0);
        free (points);
    }
    if (mesh->HasNormals ()) {
        GLuint vbo;
        glGenBuffers (1, &vbo);
        glBindBuffer (GL_ARRAY_BUFFER, vbo);
        glBufferData (
            GL_ARRAY_BUFFER,
            3 * vert_no * sizeof (GLfloat),
            normals,
            GL_STATIC_DRAW
        );
        glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray (1);
        free (normals);
    }
    if (mesh->HasTextureCoords (0)) {
        GLuint vbo;
        glGenBuffers (1, &vbo);
        glBindBuffer (GL_ARRAY_BUFFER, vbo);
        glBufferData (
            GL_ARRAY_BUFFER,
            2 * vert_no * sizeof (GLfloat),
            texcoords,
            GL_STATIC_DRAW
        );
        glVertexAttribPointer (2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray (2);
        free (texcoords);
    }
    if (mesh->HasTangentsAndBitangents ()) {
        // NB: could store/print tangents here
    }

    aiReleaseImport (scene);
    printf ("mesh loaded\n");

    return true;
}

btRigidBody* GameObject::getRigidBody(){
    return this->rigidBody;
}

void GameObject::setMass(btScalar masa)
{
    this->mass = masa;
}

btScalar GameObject::getMass()
{
    return this->mass;
}

btVector3 GameObject::getPosition()
{
    return this->position;
}

void GameObject::setPosition(btVector3 posicion)
{
    this->position = posicion;
}

btQuaternion GameObject::getRotation()
{
    return this->rotation;
}
void GameObject::setRotation(btQuaternion rotacion)
{
    this->rotation = rotation;
}

GLuint GameObject::getVao(){
    return this->vao;
}

int GameObject::getNumVertices(){
    return this->vertNumber;
}

void GameObject::setModelMatrix(glm::mat4 model){
    this->modelMatrix = model;
}



void GameObject::setWorld(btDiscreteDynamicsWorld* world){
    this->world = world;
}

btDiscreteDynamicsWorld* GameObject::getWorld()
{
    return this->world;
}

void GameObject::draw(GLuint model_mat_location){
    /*
    btTransform trans;
    this -> getRigidBody()->getMotionState()->getWorldTransform(trans);
    trans.getOpenGLMatrix(&modelMatrix[0][0]);
    this -> setModelMatrix(this->modelMatrix);
    glUniformMatrix4fv(matloc, 1, GL_FALSE, &this->modelMatrix[0][0]);
    glBindVertexArray(this->getVao());
    glDrawArrays(GL_TRIANGLES, 0, this->getNumVertices());
    */
   btTransform trans;
    this -> getRigidBody()->getMotionState()->getWorldTransform(trans);
    trans.getOpenGLMatrix(&model[0][0]);
    this -> setModelMatrix(this->model);
    glUniformMatrix4fv(model_mat_location, 1, GL_FALSE, &modelMatrix[0][0]);
    
        glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, this->texture);
    glUniform1i (tex_location, 0);
    
    glActiveTexture (GL_TEXTURE1);
	glBindTexture (GL_TEXTURE_2D, this->normalMap);
    //glUniform1i (normalMapLocation, 1);
    
    glBindVertexArray(this->getVao());
    glDrawArrays(GL_TRIANGLES, 0, this -> getNumVertices());
    
}

bool GameObject::load_texture (GLuint shaderprog, const char* texture_path, const char* normal_path){
		int x, y, n;
		int force_channels = 4;
		unsigned char* image_data = stbi_load (texture_path, &x, &y, &n, force_channels);
		if (!image_data) {
			fprintf (stderr, "ERROR: could not load %s\n", texture_path);
		}
	
		if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) 
			fprintf (stderr, "WARNING: texture %s is not power-of-2 dimensions: %i, %i\n", texture_path, x, y);
	
		int width_in_bytes = x * 4;
		unsigned char *top = NULL;
		unsigned char *bottom = NULL;
		unsigned char temp = 0;
		int half_height = y / 2;
	
		for(int row = 0; row < half_height; row++) {
			top = image_data + row * width_in_bytes;
			bottom = image_data + (y - row - 1) * width_in_bytes;
			for(int col = 0; col < width_in_bytes; col++){
				temp = *top;
				*top = *bottom;
				*bottom = temp;
				top++;
				bottom++;
			}
		}
	
		texture = 0;
		glGenTextures(1, &texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
		
	
		tex_location = glGetUniformLocation (shaderprog, "basic_texture");
		
		free(image_data);

		if (normal_path != NULL) {
			x, y, n;
			force_channels = 4;
			unsigned char* image_data2 = stbi_load (normal_path, &x, &y, &n, force_channels);
			if (!image_data2) {
				fprintf (stderr, "ERROR: could not load %s\n", normal_path);
			}
		
			if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) 
				fprintf (stderr, "WARNING: texture %s is not power-of-2 dimensions: %i, %i\n", normal_path, x, y);
		
			width_in_bytes = x * 4;
			top = NULL;
			bottom = NULL;
			temp = 0;
			half_height = y / 2;
		
			for(int row = 0; row < half_height; row++) {
				top = image_data2 + row * width_in_bytes;
				bottom = image_data2 + (y - row - 1) * width_in_bytes;
				for(int col = 0; col < width_in_bytes; col++){
					temp = *top;
					*top = *bottom;
					*bottom = temp;
					top++;
					bottom++;
				}
			}
		
			glActiveTexture(GL_TEXTURE1);			
			normalMap = 0;
			glGenTextures(1, &normalMap);
			glBindTexture(GL_TEXTURE_2D, normalMap);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data2);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			
			normalMapLoc = glGetUniformLocation (shaderprog, "normal_map");

			free(image_data2);			
		}
		return true;
}

bool GameObject::load_texture2 (GLuint shaderprog, const char* texture_path, GLuint& texture, GLuint tex_location){

	int x, y, n;
	int force_channels = 4;
	unsigned char* image_data = stbi_load (texture_path, &x, &y, &n, force_channels);
	if (!image_data) {
		fprintf (stderr, "ERROR: could not load %s\n", texture_path);
	}

	if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) 
		fprintf (stderr, "WARNING: texture %s is not power-of-2 dimensions: %i, %i\n", texture_path, x, y);

	int width_in_bytes = x * 4;
	unsigned char *top = NULL;
	unsigned char *bottom = NULL;
	unsigned char temp = 0;
	int half_height = y / 2;

	for(int row = 0; row < half_height; row++) {
		top = image_data + row * width_in_bytes;
		bottom = image_data + (y - row - 1) * width_in_bytes;
		for(int col = 0; col < width_in_bytes; col++){
			temp = *top;
			*top = *bottom;
			*bottom = temp;
			top++;
			bottom++;
		}
	}

	texture = 0;
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	tex_location = glGetUniformLocation (shaderprog, "basic_texture");
	//LIBERAR IMAGE DATA
	return true;
}