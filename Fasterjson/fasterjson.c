#include "fasterjson.h"

char g_fasterjson_encoding = FASTERJSON_ENCODING_UTF8;

int __FASTERJSON_VERSION_1_1_6 = 0;

static int _TravelJsonArrayBuffer( char top , register char **json_ptr , char *jpath , int jpath_len , int jpath_size
	, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonBranch
	, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonBranch
	, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonArray
	, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonArray
	, funcCallbackOnJsonNode *pfuncCallbackOnJsonLeaf
	, void *p , char *array_nodename , int array_nodename_len );
static int _TravelJsonLeafBuffer( char top , register char **json_ptr , char *jpath , int jpath_len , int jpath_size
	, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonBranch
	, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonBranch
	, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonArray
	, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonArray
	, funcCallbackOnJsonNode *pfuncCallbackOnJsonLeaf
	, void *p );
static int _TravelJsonBuffer( char top , register char **json_ptr , char *jpath , int jpath_len , int jpath_size
	, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonBranch
	, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonBranch
	, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonArray
	, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonArray
	, funcCallbackOnJsonNode *pfuncCallbackOnJsonLeaf
	, void *p );

static int _TravelJsonArrayBuffer( char top , register char **json_ptr , char *jpath , int jpath_len , int jpath_size
	, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonBranch
	, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonBranch
	, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonArray
	, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonArray
	, funcCallbackOnJsonNode *pfuncCallbackOnJsonLeaf
	, void *p , char *array_nodename , int array_nodename_len )
{
	char		*begin = NULL ;
	int		len = 0 ;
	signed char	tag ;
	signed char	quotes ;
	
	char		*nodename = NULL ;
	int		nodename_len ;
	char		*content = NULL ;
	int		content_len ;
	
	int		jpath_newlen = 0 ;
	
	char		first_node ;
	
	int		nret = 0 ;
	
	first_node = 1 ;
	while(1)
	{
		TOKENJSON(*json_ptr,begin,len,tag,quotes,FASTERJSON_ERROR_END_OF_BUFFER)
		if( tag == FASTERJSON_TOKEN_TEXT || tag == FASTERJSON_TOKEN_SPECIAL )
		{
			if( quotes == '\'' )
				return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_1;
			content = begin ;
			content_len = len ;
			
			if( jpath )
			{
				if( jpath_len + 1 + 1 < jpath_size-1 - 1 )
				{
					memcpy( jpath + jpath_len , "/." , 2 );
					jpath_newlen = jpath_len + 1 + 1 ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else if( jpath_len + 1 + 1 <= jpath_size-1 )
				{
					memcpy( jpath + jpath_len , "/*" , 2 );
					jpath_newlen = jpath_len + 1 + 1 ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else
				{
					jpath_newlen = jpath_len ;
				}
			}
			
			if( pfuncCallbackOnJsonLeaf )
			{
				nret = (*pfuncCallbackOnJsonLeaf)( FASTERJSON_NODE_LEAF , jpath , jpath_newlen , jpath_size , NULL , 0 , content , content_len , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
		}
		else if( tag == FASTERJSON_TOKEN_LBB )
		{
			if( pfuncCallbackOnEnterJsonBranch )
			{
				nret = (*pfuncCallbackOnEnterJsonBranch)( FASTERJSON_NODE_ENTER | FASTERJSON_NODE_BRANCH , jpath , jpath_len , jpath_size , array_nodename , array_nodename_len , NULL , 0 , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
			
			nret = _TravelJsonLeafBuffer( '{' , json_ptr , jpath , jpath_len , jpath_size
						, pfuncCallbackOnEnterJsonBranch
						, pfuncCallbackOnLeaveJsonBranch
						, pfuncCallbackOnEnterJsonArray
						, pfuncCallbackOnLeaveJsonArray
						, pfuncCallbackOnJsonLeaf
						, p ) ;
			if( nret )
				return nret;
			
			if( pfuncCallbackOnLeaveJsonBranch )
			{
				nret = (*pfuncCallbackOnLeaveJsonBranch)( FASTERJSON_NODE_LEAVE | FASTERJSON_NODE_BRANCH , jpath , jpath_len , jpath_size , array_nodename , array_nodename_len , NULL , 0 , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
		}
		else if( tag == FASTERJSON_TOKEN_LSB )
		{
			nodename = begin ;
			nodename_len = len ;
			
			if( pfuncCallbackOnEnterJsonArray )
			{
				nret = (*pfuncCallbackOnEnterJsonArray)( FASTERJSON_NODE_ENTER | FASTERJSON_NODE_ARRAY , jpath , jpath_newlen , jpath_size , nodename , nodename_len , NULL , 0 , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
			
			nret = _TravelJsonArrayBuffer( '[' , json_ptr , jpath , jpath_newlen , jpath_size
						, pfuncCallbackOnEnterJsonBranch
						, pfuncCallbackOnLeaveJsonBranch
						, pfuncCallbackOnEnterJsonArray
						, pfuncCallbackOnLeaveJsonArray
						, pfuncCallbackOnJsonLeaf
						, p , nodename , nodename_len ) ;
			if( nret )
				return nret;
			
			if( pfuncCallbackOnLeaveJsonArray )
			{
				nret = (*pfuncCallbackOnLeaveJsonArray)( FASTERJSON_NODE_LEAVE | FASTERJSON_NODE_ARRAY , jpath , jpath_newlen , jpath_size , nodename , nodename_len , NULL , 0 , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
			
			if( jpath )
			{
				if( jpath_len + 1 + nodename_len < jpath_size-1 - 1 )
				{
					*( jpath + jpath_len ) = '/' ;
					memcpy( jpath + jpath_len + 1 , nodename , (int)nodename_len );
					jpath_newlen = jpath_len + 1 + nodename_len ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else if( jpath_len + 1 + 1 <= jpath_size-1 )
				{
					memcpy( jpath + jpath_len , "/*" , 2 );
					jpath_newlen = jpath_len + 1 + 1 ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else
				{
					jpath_newlen = jpath_len ;
				}
			}
		}
		else if( tag == FASTERJSON_TOKEN_RSB )
		{
			if( first_node == 1 )
				break;
			else
				return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_ARRAY_1;
		}
		else
		{
			return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_ARRAY_1;
		}
		
		TOKENJSON(*json_ptr,begin,len,tag,quotes,FASTERJSON_ERROR_END_OF_BUFFER)
		if( tag == FASTERJSON_TOKEN_COMMA )
		{
		}
		else if( tag == FASTERJSON_TOKEN_RSB )
		{
			break;
		}
		else if( tag == FASTERJSON_TOKEN_RBB )
		{
			return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_ARRAY_2;
		}
		else
		{
			return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_ARRAY_2;
		}
		
		first_node = 0 ;
	}
	
	return 0;
}

static int _TravelJsonLeafBuffer( char top , register char **json_ptr , char *jpath , int jpath_len , int jpath_size
	, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonBranch
	, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonBranch
	, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonArray
	, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonArray
	, funcCallbackOnJsonNode *pfuncCallbackOnJsonLeaf
	, void *p )
{
	char		*begin = NULL ;
	int		len = 0 ;
	signed char	tag ;
	signed char	quotes , quotes_bak ;
	
	char		*nodename = NULL ;
	int		nodename_len ;
	char		*content = NULL ;
	int		content_len ;
	
	int		jpath_newlen = 0 ;
	
	char		first_node ;
	
	int		nret = 0 ;
	
	first_node = 1 ;
	while(1)
	{
		TOKENJSON(*json_ptr,begin,len,tag,quotes,FASTERJSON_ERROR_END_OF_BUFFER)
		if( tag == FASTERJSON_TOKEN_RBB )
		{
			if( first_node == 1 )
				break;
			else
				return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_1;
		}
		else if( tag != FASTERJSON_TOKEN_TEXT )
			return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_1;
		quotes_bak = quotes ;
		
		nodename = begin ;
		nodename_len = len ;
		
		TOKENJSON(*json_ptr,begin,len,tag,quotes,FASTERJSON_ERROR_END_OF_BUFFER)
		if( tag == FASTERJSON_TOKEN_COLON )
		{
			if( quotes_bak != '\"' )
				return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_1;
		}
		else if( tag == FASTERJSON_TOKEN_COMMA )
		{
			if( top != '[' )
				return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_1;
			
			content = begin ;
			content_len = len ;
			
			if( jpath )
			{
				if( jpath_len + 1 + 1 < jpath_size-1 - 1 )
				{
					memcpy( jpath + jpath_len , "/." , 2 );
					jpath_newlen = jpath_len + 1 + 1 ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else if( jpath_len + 1 + 1 <= jpath_size-1 )
				{
					memcpy( jpath + jpath_len , "/*" , 2 );
					jpath_newlen = jpath_len + 1 + 1 ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else
				{
					jpath_newlen = jpath_len ;
				}
			}
			
			if( pfuncCallbackOnJsonLeaf )
			{
				nret = (*pfuncCallbackOnJsonLeaf)( FASTERJSON_NODE_LEAF , jpath , jpath_newlen , jpath_size , NULL , 0 , content , content_len , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
			
			continue;
		}
		else if( tag == FASTERJSON_TOKEN_RBB )
		{
			content = begin ;
			content_len = len ;
			
			if( jpath )
			{
				if( jpath_len + 1 + 1 < jpath_size-1 - 1 )
				{
					memcpy( jpath + jpath_len , "/." , 2 );
					jpath_newlen = jpath_len + 1 + 1 ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else if( jpath_len + 1 + 1 <= jpath_size-1 )
				{
					memcpy( jpath + jpath_len , "/*" , 2 );
					jpath_newlen = jpath_len + 1 + 1 ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else
				{
					jpath_newlen = jpath_len ;
				}
			}
			
			if( pfuncCallbackOnJsonLeaf )
			{
				nret = (*pfuncCallbackOnJsonLeaf)( FASTERJSON_NODE_LEAF , jpath , jpath_newlen , jpath_size , NULL , 0 , content , content_len , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
			
			break;
		}
		else if( tag == FASTERJSON_TOKEN_RSB )
		{
			content = begin ;
			content_len = len ;
			
			if( jpath )
			{
				if( jpath_len + 1 + 1 < jpath_size-1 - 1 )
				{
					memcpy( jpath + jpath_len , "/." , 2 );
					jpath_newlen = jpath_len + 1 + 1 ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else if( jpath_len + 1 + 1 <= jpath_size-1 )
				{
					memcpy( jpath + jpath_len , "/*" , 2 );
					jpath_newlen = jpath_len + 1 + 1 ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else
				{
					jpath_newlen = jpath_len ;
				}
			}
			
			if( pfuncCallbackOnJsonLeaf )
			{
				nret = (*pfuncCallbackOnJsonLeaf)( FASTERJSON_NODE_LEAF , jpath , jpath_newlen , jpath_size , NULL , 0 , content , content_len , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
			
			break;
		}
		else
		{
			return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_2;
		}
		
		if( jpath )
		{
			if( jpath_len + 1 + nodename_len < jpath_size-1 - 1 )
			{
				*( jpath + jpath_len ) = '/' ;
				memcpy( jpath + jpath_len + 1 , nodename , (int)nodename_len );
				jpath_newlen = jpath_len + 1 + nodename_len ;
				*( jpath + jpath_newlen ) = '\0' ;
			}
			else if( jpath_len + 1 + 1 <= jpath_size-1 )
			{
				memcpy( jpath + jpath_len , "/*" , 2 );
				jpath_newlen = jpath_len + 1 + 1 ;
				*( jpath + jpath_newlen ) = '\0' ;
			}
			else
			{
				jpath_newlen = jpath_len ;
			}
		}
		
		TOKENJSON(*json_ptr,begin,len,tag,quotes,FASTERJSON_ERROR_END_OF_BUFFER)
		if( tag == FASTERJSON_TOKEN_TEXT || tag == FASTERJSON_TOKEN_SPECIAL )
		{
			if( quotes == '\'' )
				return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_3;
			
			content = begin ;
			content_len = len ;
			
			if( pfuncCallbackOnJsonLeaf )
			{
				nret = (*pfuncCallbackOnJsonLeaf)( FASTERJSON_NODE_LEAF , jpath , jpath_newlen , jpath_size , nodename , nodename_len , content , content_len , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
		}
		else if( tag == FASTERJSON_TOKEN_LBB )
		{
			if( pfuncCallbackOnEnterJsonBranch )
			{
				nret = (*pfuncCallbackOnEnterJsonBranch)( FASTERJSON_NODE_ENTER | FASTERJSON_NODE_BRANCH , jpath , jpath_newlen , jpath_size , nodename , nodename_len , NULL , 0 , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
			
			nret = _TravelJsonLeafBuffer( '{' , json_ptr , jpath , jpath_newlen , jpath_size
						, pfuncCallbackOnEnterJsonBranch
						, pfuncCallbackOnLeaveJsonBranch
						, pfuncCallbackOnEnterJsonArray
						, pfuncCallbackOnLeaveJsonArray
						, pfuncCallbackOnJsonLeaf
						, p ) ;
			if( nret )
				return nret;
			
			if( jpath )
			{
				if( jpath_len + 1 + nodename_len < jpath_size-1 - 1 )
				{
					*( jpath + jpath_len ) = '/' ;
					memcpy( jpath + jpath_len + 1 , nodename , (int)nodename_len );
					jpath_newlen = jpath_len + 1 + nodename_len ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else if( jpath_len + 1 + 1 <= jpath_size-1 )
				{
					memcpy( jpath + jpath_len , "/*" , 2 );
					jpath_newlen = jpath_len + 1 + 1 ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else
				{
					jpath_newlen = jpath_len ;
				}
			}
			
			if( pfuncCallbackOnLeaveJsonBranch )
			{
				nret = (*pfuncCallbackOnLeaveJsonBranch)( FASTERJSON_NODE_LEAVE | FASTERJSON_NODE_BRANCH , jpath , jpath_newlen , jpath_size , nodename , nodename_len , NULL , 0 , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
		}
		else if( tag == FASTERJSON_TOKEN_LSB )
		{
			nodename = begin ;
			nodename_len = len ;
			
			if( pfuncCallbackOnEnterJsonArray )
			{
				nret = (*pfuncCallbackOnEnterJsonArray)( FASTERJSON_NODE_ENTER | FASTERJSON_NODE_ARRAY , jpath , jpath_newlen , jpath_size , nodename , nodename_len , NULL , 0 , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
			
			nret = _TravelJsonArrayBuffer( '[' , json_ptr , jpath , jpath_newlen , jpath_size
						, pfuncCallbackOnEnterJsonBranch
						, pfuncCallbackOnLeaveJsonBranch
						, pfuncCallbackOnEnterJsonArray
						, pfuncCallbackOnLeaveJsonArray
						, pfuncCallbackOnJsonLeaf
						, p , nodename , nodename_len ) ;
			if( nret )
				return nret;
			
			if( pfuncCallbackOnLeaveJsonArray )
			{
				nret = (*pfuncCallbackOnLeaveJsonArray)( FASTERJSON_NODE_LEAVE | FASTERJSON_NODE_ARRAY , jpath , jpath_newlen , jpath_size , nodename , nodename_len , NULL , 0 , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
			
			if( jpath )
			{
				if( jpath_len + 1 + nodename_len < jpath_size-1 - 1 )
				{
					*( jpath + jpath_len ) = '/' ;
					memcpy( jpath + jpath_len + 1 , nodename , (int)nodename_len );
					jpath_newlen = jpath_len + 1 + nodename_len ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else if( jpath_len + 1 + 1 <= jpath_size-1 )
				{
					memcpy( jpath + jpath_len , "/*" , 2 );
					jpath_newlen = jpath_len + 1 + 1 ;
					*( jpath + jpath_newlen ) = '\0' ;
				}
				else
				{
					jpath_newlen = jpath_len ;
				}
			}
		}
		else
		{
			return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_3;
		}
		
		TOKENJSON(*json_ptr,begin,len,tag,quotes,FASTERJSON_ERROR_END_OF_BUFFER)
		if( tag == FASTERJSON_TOKEN_COMMA )
		{
		}
		else if( tag == FASTERJSON_TOKEN_RBB )
		{
			break;
		}
		else
		{
			return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_4;
		}
		
		first_node = 0 ;
	}
	
	return 0;
}

static int _TravelJsonBuffer( char top , register char **json_ptr , char *jpath , int jpath_len , int jpath_size
	, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonBranch
	, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonBranch
	 , funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonArray
	 , funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonArray
	, funcCallbackOnJsonNode *pfuncCallbackOnJsonLeaf
	, void *p )
{
	char		*begin = NULL ;
	int		len = 0 ;
	signed char	tag ;
	signed char	quotes ;
	
	char		*nodename ;
	int		nodename_len ;
	
	int		nret = 0 ;
	
	while(1)
	{
		TOKENJSON(*json_ptr,begin,len,tag,quotes,FASTERJSON_INFO_END_OF_BUFFER)
		if( tag == FASTERJSON_TOKEN_LBB )
		{
			nodename = begin ;
			nodename_len = len ;
			
			if( pfuncCallbackOnEnterJsonBranch )
			{
				nret = (*pfuncCallbackOnEnterJsonBranch)( FASTERJSON_NODE_ENTER | FASTERJSON_NODE_BRANCH , jpath , jpath_len , jpath_size , nodename , nodename_len , NULL , 0 , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
			
			nret = _TravelJsonLeafBuffer( '{' , json_ptr , jpath , jpath_len , jpath_size
						, pfuncCallbackOnEnterJsonBranch
						, pfuncCallbackOnLeaveJsonBranch
						, pfuncCallbackOnEnterJsonArray
						, pfuncCallbackOnLeaveJsonArray
						, pfuncCallbackOnJsonLeaf
						, p ) ;
			if( nret )
				return nret;
			
			if( pfuncCallbackOnLeaveJsonBranch )
			{
				nret = (*pfuncCallbackOnLeaveJsonBranch)( FASTERJSON_NODE_LEAVE | FASTERJSON_NODE_BRANCH , jpath , jpath_len , jpath_size , nodename , nodename_len , NULL , 0 , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
		}
		else if( tag == FASTERJSON_TOKEN_LSB )
		{
			nodename = begin ;
			nodename_len = len ;
			
			if( pfuncCallbackOnEnterJsonArray )
			{
				nret = (*pfuncCallbackOnEnterJsonArray)( FASTERJSON_NODE_ENTER | FASTERJSON_NODE_ARRAY , jpath , jpath_len , jpath_size , nodename , nodename_len , NULL , 0 , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
			
			nret = _TravelJsonArrayBuffer( '[' , json_ptr , jpath , jpath_len , jpath_size
						, pfuncCallbackOnEnterJsonBranch
						, pfuncCallbackOnLeaveJsonBranch
						, pfuncCallbackOnEnterJsonArray
						, pfuncCallbackOnLeaveJsonArray
						, pfuncCallbackOnJsonLeaf
						, p , nodename , nodename_len ) ;
			if( nret )
				return nret;
			
			if( pfuncCallbackOnLeaveJsonArray )
			{
				nret = (*pfuncCallbackOnLeaveJsonArray)( FASTERJSON_NODE_LEAVE | FASTERJSON_NODE_ARRAY , jpath , jpath_len , jpath_size , nodename , nodename_len , NULL , 0 , p ) ;
				if( nret > 0 )
					break;
				else if( nret < 0 )
					return nret;
			}
		}
		else
		{
			return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_0;
		}
	}
	
	return 0;
}

int TravelJsonBuffer( char *json_buffer , char *jpath , int jpath_size
		    , funcCallbackOnJsonNode *pfuncCallbackOnJsonNode
		    , void *p )
{
	char		*json_ptr = json_buffer ;
	
	int		nret = 0 ;
	
	nret = _TravelJsonBuffer( '\0' , & json_ptr , jpath , 0 , jpath_size
				, pfuncCallbackOnJsonNode
				, pfuncCallbackOnJsonNode
				, pfuncCallbackOnJsonNode
				, pfuncCallbackOnJsonNode
				, pfuncCallbackOnJsonNode
				, p ) ;
	if( nret == 0 || nret == FASTERJSON_INFO_END_OF_BUFFER )
		return 0;
	else
		return nret;
}

int TravelJsonBuffer4( char *json_buffer , char *jpath , int jpath_size
		, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonBranch
		, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonBranch
		, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonArray
		, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonArray
		, funcCallbackOnJsonNode *pfuncCallbackOnJsonLeaf
		, void *p )
{
	char		*json_ptr = json_buffer ;
	
	int		nret = 0 ;
	
	nret = _TravelJsonBuffer( '\0' , & json_ptr , jpath , 0 , jpath_size
				, pfuncCallbackOnEnterJsonBranch
				, pfuncCallbackOnLeaveJsonBranch
				, pfuncCallbackOnEnterJsonArray
				, pfuncCallbackOnLeaveJsonArray
				, pfuncCallbackOnJsonLeaf
				, p ) ;
	if( nret == 0 || nret == FASTERJSON_INFO_END_OF_BUFFER )
		return 0;
	else
		return nret;
}

