
#include "brw_context.h"
#include "brw_state.h"
#include "pipe/p_util.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/tgsi/util/tgsi_parse.h"




void brw_shader_info(const struct tgsi_token *tokens,
		     struct brw_shader_info *info )
{
   struct tgsi_parse_context parse;
   int done = 0;

   tgsi_parse_init( &parse, tokens );

   while( !done &&
	  !tgsi_parse_end_of_tokens( &parse ) ) 
   {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
      {
	 const struct tgsi_full_declaration *decl = &parse.FullToken.FullDeclaration;
	 unsigned last = decl->u.DeclarationRange.Last;
	 
	 assert( decl->Declaration.Declare == TGSI_DECLARE_RANGE );
      
	 // Broken by crazy wpos init:
	 //assert( info->nr_regs[decl->Declaration.File] <= last);

	 info->nr_regs[decl->Declaration.File] = MAX2(info->nr_regs[decl->Declaration.File],
						      last+1);
	 break;
      }
      case TGSI_TOKEN_TYPE_IMMEDIATE:
      case TGSI_TOKEN_TYPE_INSTRUCTION:
      default:
	 done = 1;
	 break;
      }
   }

   tgsi_parse_free (&parse);
   
}
