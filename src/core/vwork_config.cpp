#include "vwork_config.h"

/*
 * 集中定义各 config 的专属栈内存。
 * 栈大小来自 vwork_config.h 的 VWORK_CONFIG_TABLE（唯一真源），
 * 不在此重复，增删坑位只改表即可。
 */

namespace vwork
{
namespace configs
{

#define VWORK_X_DEFINE(_var, _name, _sz, _prio, _model) \
	K_THREAD_STACK_DEFINE(_var##_stack, _sz);

VWORK_CONFIG_TABLE(VWORK_X_DEFINE)

#undef VWORK_X_DEFINE

} // namespace configs
} // namespace vwork
