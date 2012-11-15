#ifndef GLOBJ_H
#define GLOBJ_H
#include <QtOpenGL>

class CacheEntry {
public:
	int Vert, Tex, Norm;
	CacheEntry() { Vert = 0; Tex = 0; Norm = 0; }
	CacheEntry(int v, int t, int n) {
		Vert = v;
		Tex = t;
		Norm = n;
	}
	bool operator == (const CacheEntry &other) const {
		return (other.Norm == Norm && other.Vert == Vert && other.Tex == Tex);
	}

};

class GLObj {
public:
	QVector<QVector3D> Vertices;
	QVector<GLuint> Indices;
	size_t IndNum;
	QGLBuffer BufInt, BufFloat;
	QVector3D MinVec, MaxVec;
	bool Init;
	GLObj() {
		Init = false;
	}
	void loadFile(QString file) {
		Init = false;
		readFile(file);
		upload();
	}

	void loadFile(QString file, QVector3D scale) {
		Init = false;
		readFile(file);
		scaleAndCenter(scale);
		upload();
	}


	void draw(bool enableTexture = true, bool enableColor = false) {
		glEnableClientState( GL_VERTEX_ARRAY );
		BufFloat.bind();
		glVertexPointer(3, GL_FLOAT, 3*sizeof(QVector3D), 0);
		if (enableTexture) {
			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			glTexCoordPointer(3, GL_FLOAT,3*sizeof(QVector3D), (void *) sizeof(QVector3D));
		}
		if (enableColor) {
			glEnableClientState( GL_COLOR_ARRAY );
			glColorPointer(3, GL_FLOAT,3*sizeof(QVector3D), (void *) sizeof(QVector3D));
		}
		glNormalPointer(GL_FLOAT, 3*sizeof(QVector3D), (void *) (2*sizeof(QVector3D)));
		BufInt.bind();
		glIndexPointer(GL_UNSIGNED_INT, 2*sizeof(GLuint), 0);
		glDrawElements(GL_TRIANGLES, IndNum, GL_UNSIGNED_INT, 0);
		BufFloat.release();
		BufInt.release();
		if (enableColor) {
			glDisableClientState( GL_COLOR_ARRAY );
		}
		if (enableTexture) {
			glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		glDisableClientState( GL_VERTEX_ARRAY );
	}

	void draw(float x, float y, float z, bool enableTexture = true, bool enableColor = false) {

		//glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);

		glTranslatef( x, y, z);

		draw(enableTexture, enableColor);
		
		glTranslatef( -x, -y, -z);
	}
	void readFile(QString file) {
		QVector<GLuint> tmpInd;
		QVector<QVector3D> tmpVert, tmpTex, tmpNorm;
		QVector<CacheEntry> cache;
		int i, j, k;
		QFile f(file);
		f.open(QFile::QIODevice::ReadOnly);
		bool first = true;
		float max[3] = {0, 0, 0};
		float min[3] = {0, 0, 0};
		QRegExp reg(" +");

		while (!f.atEnd()) {
			QString str = f.readLine();
			QStringList qsl = str.split(reg);
			if (qsl.at(0) == "v") {
				float arr[3];
				for (i = 0; i < 3; i ++) {
					arr[i] = qsl.at(i+1).toFloat();

					max[i] = ((max[i] < arr[i] || !max[i] || first ) ? arr[i] : max[i]);
					min[i] = ((min[i] > arr[i] || first ) ? arr[i] : min[i]);

				}
				first = false;
				tmpVert.push_back(QVector3D(arr[0], arr[1], arr[2]));
			} else if (qsl.at(0) == "f") {
				for (i = 0; i < qsl.size()-1; i ++) {
					QStringList qsl2 = qsl.at(i+1).split("/");
					for (j =0; j < 3; j++) {
						tmpInd.push_back(qsl2.at(j).toUInt()-1);
					}
				}
			} else if (qsl.at(0) == "vt") {
				tmpTex.push_back(QVector3D(qsl.at(1).toFloat(), qsl.at(2).toFloat(), qsl.at(3).toFloat()));
			} else if (qsl.at(0) == "vn") {
				tmpNorm.push_back(QVector3D(qsl.at(1).toFloat(), qsl.at(2).toFloat(), qsl.at(3).toFloat()));
			}
		}
		f.close();
		bool found;
		k = 0;
		for (int i = 0; i < tmpInd.size(); i+=3) {
			found = false;
			CacheEntry tmpEntry = CacheEntry(tmpInd[i], tmpInd[i+1], tmpInd[i+2]);
			for (j = 0; j < cache.size(); j++) {
				if (cache[j] == tmpEntry) {
					found = true;
					break;
				}
			}
			if (!found) {
				Vertices.push_back(tmpVert[tmpInd[i]]);
				Vertices.push_back(tmpTex[tmpInd[i+1]]);
				Vertices.push_back(tmpNorm[tmpInd[i+2]]);
				Indices.push_back(k);
				cache.push_back(tmpEntry);
				k++;
			} else {
				Indices.push_back(j);
			}
		}
		MinVec = QVector3D(min[0], min[1], min[2]);
		MaxVec = QVector3D(max[0], max[1], max[2]);
	}
	void scaleAndCenter(QVector3D scale) {
		int i;
		QVector3D diff = MaxVec - MinVec;

		if ((scale - diff).length() < pow(10,-10)) {
			return;
		}
		QVector3D scaleCorrected = QVector3D(1./diff.x(), 1./diff.y(), 1./diff.z()) * scale;
		QVector3D offset = QVector3D(0.,0.,0.);
		for (i = 0; i < Vertices.size(); i+=3) {
			Vertices[i] = (Vertices[i] - MinVec)*scaleCorrected+offset;
		}
		MinVec = QVector3D(0,0,0);
		MaxVec = scale;
	}
	void upload() {
		if (!Init) {
			BufFloat = QGLBuffer(QGLBuffer::VertexBuffer);
			BufInt = QGLBuffer(QGLBuffer::IndexBuffer);
			BufFloat.create();
			BufFloat.bind();
			BufFloat.allocate(sizeof(QVector3D)*Vertices.size());
			BufInt.create();
			BufInt.bind();
			BufInt.allocate(sizeof(GLuint)*Indices.size());
			Init = true;
		}

		uploadVert();
		uploadInd();

	}
	void uploadVert() {
		BufFloat.bind();
		BufFloat.write(0, &Vertices[0], sizeof(QVector3D)*Vertices.size());
		BufFloat.setUsagePattern(QGLBuffer::DynamicDraw);
	}

	void uploadInd() {
		BufInt.write(0, &Indices[0], sizeof(GLuint)*Indices.size());
		IndNum = Indices.size();
		BufInt.setUsagePattern(QGLBuffer::DynamicDraw);
	}

	void SetColorOrTexPos(int pos, QVector3D value) {
		Vertices[pos*3+1] = value;
	}
	void SetColorOrTexPos(QVector3D pos, QVector3D value) {
		for (int i = 0; i < Vertices.size()/3; i++) {
			if ((Vertices[i*3]-pos).length() < 10e-14) {
				Vertices[i*3+1] = value;
			}
		}
	}
};

#endif // GLOBJ_H
