// A XML parser to read and setup a level - uses TinyXML2 to parse the file into its own structure, the 
// methods in this class traverse that structure and create entities and templates as appropriate
#ifndef GEN_C_PARSE_LEVEL_H_INCLUDED
#define GEN_C_PARSE_LEVEL_H_INCLUDED

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "EntityManager.h"

#include "tinyxml2.h"

namespace gen
{

/*---------------------------------------------------------------------------------------------
	CParseLevel class
---------------------------------------------------------------------------------------------*/
class CParseLevel
{

/*---------------------------------------------------------------------------------------------
	Constructors / Destructors
---------------------------------------------------------------------------------------------*/
public:
	// Constructor just stores a pointer to the entity manager so all methods below can access it
	CParseLevel( CEntityManager* entityManager ) : m_EntityManager(entityManager)
	{}


	/*-----------------------------------------------------------------------------------------
		Public interface
	-----------------------------------------------------------------------------------------*/
public:
	bool ParseFile(const string& fileName);


	/*-----------------------------------------------------------------------------------------
		Private interface
	-----------------------------------------------------------------------------------------*/
private:

	bool ParseLevelElement    (tinyxml2::XMLElement* rootElement);
	bool ParseTemplatesElement(tinyxml2::XMLElement* rootElement);
	bool ParseEntitiesElement (tinyxml2::XMLElement* rootElement);

	CVector3 GetVector3FromElement(tinyxml2::XMLElement* rootElement);


	/*---------------------------------------------------------------------------------------------
		Data
	---------------------------------------------------------------------------------------------*/

	// Constructer is passed a pointer to an entity manager used to create templates and
	// entities as they are parsed
	CEntityManager* m_EntityManager;
};


} // namespace gen

#endif // GEN_C_PARSE_LEVEL_H_INCLUDED
