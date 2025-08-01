//===- ir.c - Simple test of C APIs ---------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM
// Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/* RUN: mlir-capi-ir-test 2>&1 | FileCheck %s
 */

#include "mlir-c/IR.h"
#include "mlir-c/AffineExpr.h"
#include "mlir-c/AffineMap.h"
#include "mlir-c/BuiltinAttributes.h"
#include "mlir-c/BuiltinTypes.h"
#include "mlir-c/Diagnostics.h"
#include "mlir-c/Dialect/Func.h"
#include "mlir-c/IntegerSet.h"
#include "mlir-c/RegisterEverything.h"
#include "mlir-c/Support.h"

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void registerAllUpstreamDialects(MlirContext ctx) {
  MlirDialectRegistry registry = mlirDialectRegistryCreate();
  mlirRegisterAllDialects(registry);
  mlirContextAppendDialectRegistry(ctx, registry);
  mlirDialectRegistryDestroy(registry);
}

struct ResourceDeleteUserData {
  const char *name;
};
static struct ResourceDeleteUserData resourceI64BlobUserData = {
    "resource_i64_blob"};
static void reportResourceDelete(void *userData, const void *data, size_t size,
                                 size_t align) {
  fprintf(stderr, "reportResourceDelete: %s\n",
          ((struct ResourceDeleteUserData *)userData)->name);
}

void populateLoopBody(MlirContext ctx, MlirBlock loopBody,
                      MlirLocation location, MlirBlock funcBody) {
  MlirValue iv = mlirBlockGetArgument(loopBody, 0);
  MlirValue funcArg0 = mlirBlockGetArgument(funcBody, 0);
  MlirValue funcArg1 = mlirBlockGetArgument(funcBody, 1);
  MlirType f32Type =
      mlirTypeParseGet(ctx, mlirStringRefCreateFromCString("f32"));

  MlirOperationState loadLHSState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("memref.load"), location);
  MlirValue loadLHSOperands[] = {funcArg0, iv};
  mlirOperationStateAddOperands(&loadLHSState, 2, loadLHSOperands);
  mlirOperationStateAddResults(&loadLHSState, 1, &f32Type);
  MlirOperation loadLHS = mlirOperationCreate(&loadLHSState);
  mlirBlockAppendOwnedOperation(loopBody, loadLHS);

  MlirOperationState loadRHSState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("memref.load"), location);
  MlirValue loadRHSOperands[] = {funcArg1, iv};
  mlirOperationStateAddOperands(&loadRHSState, 2, loadRHSOperands);
  mlirOperationStateAddResults(&loadRHSState, 1, &f32Type);
  MlirOperation loadRHS = mlirOperationCreate(&loadRHSState);
  mlirBlockAppendOwnedOperation(loopBody, loadRHS);

  MlirOperationState addState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("arith.addf"), location);
  MlirValue addOperands[] = {mlirOperationGetResult(loadLHS, 0),
                             mlirOperationGetResult(loadRHS, 0)};
  mlirOperationStateAddOperands(&addState, 2, addOperands);
  mlirOperationStateAddResults(&addState, 1, &f32Type);
  MlirOperation add = mlirOperationCreate(&addState);
  mlirBlockAppendOwnedOperation(loopBody, add);

  MlirOperationState storeState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("memref.store"), location);
  MlirValue storeOperands[] = {mlirOperationGetResult(add, 0), funcArg0, iv};
  mlirOperationStateAddOperands(&storeState, 3, storeOperands);
  MlirOperation store = mlirOperationCreate(&storeState);
  mlirBlockAppendOwnedOperation(loopBody, store);

  MlirOperationState yieldState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("scf.yield"), location);
  MlirOperation yield = mlirOperationCreate(&yieldState);
  mlirBlockAppendOwnedOperation(loopBody, yield);
}

MlirModule makeAndDumpAdd(MlirContext ctx, MlirLocation location) {
  MlirModule moduleOp = mlirModuleCreateEmpty(location);
  MlirBlock moduleBody = mlirModuleGetBody(moduleOp);

  MlirType memrefType =
      mlirTypeParseGet(ctx, mlirStringRefCreateFromCString("memref<?xf32>"));
  MlirType funcBodyArgTypes[] = {memrefType, memrefType};
  MlirLocation funcBodyArgLocs[] = {location, location};
  MlirRegion funcBodyRegion = mlirRegionCreate();
  MlirBlock funcBody =
      mlirBlockCreate(sizeof(funcBodyArgTypes) / sizeof(MlirType),
                      funcBodyArgTypes, funcBodyArgLocs);
  mlirRegionAppendOwnedBlock(funcBodyRegion, funcBody);

  MlirAttribute funcTypeAttr = mlirAttributeParseGet(
      ctx,
      mlirStringRefCreateFromCString("(memref<?xf32>, memref<?xf32>) -> ()"));
  MlirAttribute funcNameAttr =
      mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("\"add\""));
  MlirNamedAttribute funcAttrs[] = {
      mlirNamedAttributeGet(
          mlirIdentifierGet(ctx,
                            mlirStringRefCreateFromCString("function_type")),
          funcTypeAttr),
      mlirNamedAttributeGet(
          mlirIdentifierGet(ctx, mlirStringRefCreateFromCString("sym_name")),
          funcNameAttr)};
  MlirOperationState funcState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("func.func"), location);
  mlirOperationStateAddAttributes(&funcState, 2, funcAttrs);
  mlirOperationStateAddOwnedRegions(&funcState, 1, &funcBodyRegion);
  MlirOperation func = mlirOperationCreate(&funcState);
  mlirBlockInsertOwnedOperation(moduleBody, 0, func);

  MlirType indexType =
      mlirTypeParseGet(ctx, mlirStringRefCreateFromCString("index"));
  MlirAttribute indexZeroLiteral =
      mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("0 : index"));
  MlirNamedAttribute indexZeroValueAttr = mlirNamedAttributeGet(
      mlirIdentifierGet(ctx, mlirStringRefCreateFromCString("value")),
      indexZeroLiteral);
  MlirOperationState constZeroState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("arith.constant"), location);
  mlirOperationStateAddResults(&constZeroState, 1, &indexType);
  mlirOperationStateAddAttributes(&constZeroState, 1, &indexZeroValueAttr);
  MlirOperation constZero = mlirOperationCreate(&constZeroState);
  mlirBlockAppendOwnedOperation(funcBody, constZero);

  MlirValue funcArg0 = mlirBlockGetArgument(funcBody, 0);
  MlirValue constZeroValue = mlirOperationGetResult(constZero, 0);
  MlirValue dimOperands[] = {funcArg0, constZeroValue};
  MlirOperationState dimState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("memref.dim"), location);
  mlirOperationStateAddOperands(&dimState, 2, dimOperands);
  mlirOperationStateAddResults(&dimState, 1, &indexType);
  MlirOperation dim = mlirOperationCreate(&dimState);
  mlirBlockAppendOwnedOperation(funcBody, dim);

  MlirRegion loopBodyRegion = mlirRegionCreate();
  MlirBlock loopBody = mlirBlockCreate(0, NULL, NULL);
  mlirBlockAddArgument(loopBody, indexType, location);
  mlirRegionAppendOwnedBlock(loopBodyRegion, loopBody);

  MlirAttribute indexOneLiteral =
      mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("1 : index"));
  MlirNamedAttribute indexOneValueAttr = mlirNamedAttributeGet(
      mlirIdentifierGet(ctx, mlirStringRefCreateFromCString("value")),
      indexOneLiteral);
  MlirOperationState constOneState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("arith.constant"), location);
  mlirOperationStateAddResults(&constOneState, 1, &indexType);
  mlirOperationStateAddAttributes(&constOneState, 1, &indexOneValueAttr);
  MlirOperation constOne = mlirOperationCreate(&constOneState);
  mlirBlockAppendOwnedOperation(funcBody, constOne);

  MlirValue dimValue = mlirOperationGetResult(dim, 0);
  MlirValue constOneValue = mlirOperationGetResult(constOne, 0);
  MlirValue loopOperands[] = {constZeroValue, dimValue, constOneValue};
  MlirOperationState loopState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("scf.for"), location);
  mlirOperationStateAddOperands(&loopState, 3, loopOperands);
  mlirOperationStateAddOwnedRegions(&loopState, 1, &loopBodyRegion);
  MlirOperation loop = mlirOperationCreate(&loopState);
  mlirBlockAppendOwnedOperation(funcBody, loop);

  populateLoopBody(ctx, loopBody, location, funcBody);

  MlirOperationState retState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("func.return"), location);
  MlirOperation ret = mlirOperationCreate(&retState);
  mlirBlockAppendOwnedOperation(funcBody, ret);

  MlirOperation module = mlirModuleGetOperation(moduleOp);
  mlirOperationDump(module);
  // clang-format off
  // CHECK: module {
  // CHECK:   func @add(%[[ARG0:.*]]: memref<?xf32>, %[[ARG1:.*]]: memref<?xf32>) {
  // CHECK:     %[[C0:.*]] = arith.constant 0 : index
  // CHECK:     %[[DIM:.*]] = memref.dim %[[ARG0]], %[[C0]] : memref<?xf32>
  // CHECK:     %[[C1:.*]] = arith.constant 1 : index
  // CHECK:     scf.for %[[I:.*]] = %[[C0]] to %[[DIM]] step %[[C1]] {
  // CHECK:       %[[LHS:.*]] = memref.load %[[ARG0]][%[[I]]] : memref<?xf32>
  // CHECK:       %[[RHS:.*]] = memref.load %[[ARG1]][%[[I]]] : memref<?xf32>
  // CHECK:       %[[SUM:.*]] = arith.addf %[[LHS]], %[[RHS]] : f32
  // CHECK:       memref.store %[[SUM]], %[[ARG0]][%[[I]]] : memref<?xf32>
  // CHECK:     }
  // CHECK:     return
  // CHECK:   }
  // CHECK: }
  // clang-format on

  return moduleOp;
}

struct OpListNode {
  MlirOperation op;
  struct OpListNode *next;
};
typedef struct OpListNode OpListNode;

struct ModuleStats {
  unsigned numOperations;
  unsigned numAttributes;
  unsigned numBlocks;
  unsigned numRegions;
  unsigned numValues;
  unsigned numBlockArguments;
  unsigned numOpResults;
};
typedef struct ModuleStats ModuleStats;

int collectStatsSingle(OpListNode *head, ModuleStats *stats) {
  MlirOperation operation = head->op;
  stats->numOperations += 1;
  stats->numValues += mlirOperationGetNumResults(operation);
  stats->numAttributes += mlirOperationGetNumAttributes(operation);

  unsigned numRegions = mlirOperationGetNumRegions(operation);

  stats->numRegions += numRegions;

  intptr_t numResults = mlirOperationGetNumResults(operation);
  for (intptr_t i = 0; i < numResults; ++i) {
    MlirValue result = mlirOperationGetResult(operation, i);
    if (!mlirValueIsAOpResult(result))
      return 1;
    if (mlirValueIsABlockArgument(result))
      return 2;
    if (!mlirOperationEqual(operation, mlirOpResultGetOwner(result)))
      return 3;
    if (i != mlirOpResultGetResultNumber(result))
      return 4;
    ++stats->numOpResults;
  }

  MlirRegion region = mlirOperationGetFirstRegion(operation);
  while (!mlirRegionIsNull(region)) {
    for (MlirBlock block = mlirRegionGetFirstBlock(region);
         !mlirBlockIsNull(block); block = mlirBlockGetNextInRegion(block)) {
      ++stats->numBlocks;
      intptr_t numArgs = mlirBlockGetNumArguments(block);
      stats->numValues += numArgs;
      for (intptr_t j = 0; j < numArgs; ++j) {
        MlirValue arg = mlirBlockGetArgument(block, j);
        if (!mlirValueIsABlockArgument(arg))
          return 5;
        if (mlirValueIsAOpResult(arg))
          return 6;
        if (!mlirBlockEqual(block, mlirBlockArgumentGetOwner(arg)))
          return 7;
        if (j != mlirBlockArgumentGetArgNumber(arg))
          return 8;
        ++stats->numBlockArguments;
      }

      for (MlirOperation child = mlirBlockGetFirstOperation(block);
           !mlirOperationIsNull(child);
           child = mlirOperationGetNextInBlock(child)) {
        OpListNode *node = malloc(sizeof(OpListNode));
        node->op = child;
        node->next = head->next;
        head->next = node;
      }
    }
    region = mlirRegionGetNextInOperation(region);
  }
  return 0;
}

int collectStats(MlirOperation operation) {
  OpListNode *head = malloc(sizeof(OpListNode));
  head->op = operation;
  head->next = NULL;

  ModuleStats stats;
  stats.numOperations = 0;
  stats.numAttributes = 0;
  stats.numBlocks = 0;
  stats.numRegions = 0;
  stats.numValues = 0;
  stats.numBlockArguments = 0;
  stats.numOpResults = 0;

  do {
    int retval = collectStatsSingle(head, &stats);
    if (retval) {
      free(head);
      return retval;
    }
    OpListNode *next = head->next;
    free(head);
    head = next;
  } while (head);

  if (stats.numValues != stats.numBlockArguments + stats.numOpResults)
    return 100;

  fprintf(stderr, "@stats\n");
  fprintf(stderr, "Number of operations: %u\n", stats.numOperations);
  fprintf(stderr, "Number of attributes: %u\n", stats.numAttributes);
  fprintf(stderr, "Number of blocks: %u\n", stats.numBlocks);
  fprintf(stderr, "Number of regions: %u\n", stats.numRegions);
  fprintf(stderr, "Number of values: %u\n", stats.numValues);
  fprintf(stderr, "Number of block arguments: %u\n", stats.numBlockArguments);
  fprintf(stderr, "Number of op results: %u\n", stats.numOpResults);
  // clang-format off
  // CHECK-LABEL: @stats
  // CHECK: Number of operations: 12
  // CHECK: Number of attributes: 5
  // CHECK: Number of blocks: 3
  // CHECK: Number of regions: 3
  // CHECK: Number of values: 9
  // CHECK: Number of block arguments: 3
  // CHECK: Number of op results: 6
  // clang-format on
  return 0;
}

static void printToStderr(MlirStringRef str, void *userData) {
  (void)userData;
  fwrite(str.data, 1, str.length, stderr);
}

static void printFirstOfEach(MlirContext ctx, MlirOperation operation) {
  // Assuming we are given a module, go to the first operation of the first
  // function.
  MlirRegion region = mlirOperationGetRegion(operation, 0);
  MlirBlock block = mlirRegionGetFirstBlock(region);
  MlirOperation function = mlirBlockGetFirstOperation(block);
  region = mlirOperationGetRegion(function, 0);
  MlirOperation parentOperation = function;
  block = mlirRegionGetFirstBlock(region);
  operation = mlirBlockGetFirstOperation(block);
  assert(mlirModuleIsNull(mlirModuleFromOperation(operation)));

  // Verify that parent operation and block report correctly.
  // CHECK: Parent operation eq: 1
  fprintf(stderr, "Parent operation eq: %d\n",
          mlirOperationEqual(mlirOperationGetParentOperation(operation),
                             parentOperation));
  // CHECK: Block eq: 1
  fprintf(stderr, "Block eq: %d\n",
          mlirBlockEqual(mlirOperationGetBlock(operation), block));
  // CHECK: Block parent operation eq: 1
  fprintf(
      stderr, "Block parent operation eq: %d\n",
      mlirOperationEqual(mlirBlockGetParentOperation(block), parentOperation));
  // CHECK: Block parent region eq: 1
  fprintf(stderr, "Block parent region eq: %d\n",
          mlirRegionEqual(mlirBlockGetParentRegion(block), region));

  // In the module we created, the first operation of the first function is
  // an "memref.dim", which has an attribute and a single result that we can
  // use to test the printing mechanism.
  mlirBlockPrint(block, printToStderr, NULL);
  fprintf(stderr, "\n");
  fprintf(stderr, "First operation: ");
  mlirOperationPrint(operation, printToStderr, NULL);
  fprintf(stderr, "\n");
  // clang-format off
  // CHECK:   %[[C0:.*]] = arith.constant 0 : index
  // CHECK:   %[[DIM:.*]] = memref.dim %{{.*}}, %[[C0]] : memref<?xf32>
  // CHECK:   %[[C1:.*]] = arith.constant 1 : index
  // CHECK:   scf.for %[[I:.*]] = %[[C0]] to %[[DIM]] step %[[C1]] {
  // CHECK:     %[[LHS:.*]] = memref.load %{{.*}}[%[[I]]] : memref<?xf32>
  // CHECK:     %[[RHS:.*]] = memref.load %{{.*}}[%[[I]]] : memref<?xf32>
  // CHECK:     %[[SUM:.*]] = arith.addf %[[LHS]], %[[RHS]] : f32
  // CHECK:     memref.store %[[SUM]], %{{.*}}[%[[I]]] : memref<?xf32>
  // CHECK:   }
  // CHECK: return
  // CHECK: First operation: {{.*}} = arith.constant 0 : index
  // clang-format on

  // Get the operation name and print it.
  MlirIdentifier ident = mlirOperationGetName(operation);
  MlirStringRef identStr = mlirIdentifierStr(ident);
  fprintf(stderr, "Operation name: '");
  for (size_t i = 0; i < identStr.length; ++i)
    fputc(identStr.data[i], stderr);
  fprintf(stderr, "'\n");
  // CHECK: Operation name: 'arith.constant'

  // Get the identifier again and verify equal.
  MlirIdentifier identAgain = mlirIdentifierGet(ctx, identStr);
  fprintf(stderr, "Identifier equal: %d\n",
          mlirIdentifierEqual(ident, identAgain));
  // CHECK: Identifier equal: 1

  // Get the block terminator and print it.
  MlirOperation terminator = mlirBlockGetTerminator(block);
  fprintf(stderr, "Terminator: ");
  mlirOperationPrint(terminator, printToStderr, NULL);
  fprintf(stderr, "\n");
  // CHECK: Terminator: func.return

  // Get the attribute by name.
  bool hasValueAttr = mlirOperationHasInherentAttributeByName(
      operation, mlirStringRefCreateFromCString("value"));
  if (hasValueAttr)
    // CHECK: Has attr "value"
    fprintf(stderr, "Has attr \"value\"");

  MlirAttribute valueAttr0 = mlirOperationGetInherentAttributeByName(
      operation, mlirStringRefCreateFromCString("value"));
  fprintf(stderr, "Get attr \"value\": ");
  mlirAttributePrint(valueAttr0, printToStderr, NULL);
  fprintf(stderr, "\n");
  // CHECK: Get attr "value": 0 : index

  // Get a non-existing attribute and assert that it is null (sanity).
  fprintf(stderr, "does_not_exist is null: %d\n",
          mlirAttributeIsNull(mlirOperationGetDiscardableAttributeByName(
              operation, mlirStringRefCreateFromCString("does_not_exist"))));
  // CHECK: does_not_exist is null: 1

  // Get result 0 and its type.
  MlirValue value = mlirOperationGetResult(operation, 0);
  fprintf(stderr, "Result 0: ");
  mlirValuePrint(value, printToStderr, NULL);
  fprintf(stderr, "\n");
  fprintf(stderr, "Value is null: %d\n", mlirValueIsNull(value));
  // CHECK: Result 0: {{.*}} = arith.constant 0 : index
  // CHECK: Value is null: 0

  MlirType type = mlirValueGetType(value);
  fprintf(stderr, "Result 0 type: ");
  mlirTypePrint(type, printToStderr, NULL);
  fprintf(stderr, "\n");
  // CHECK: Result 0 type: index

  // Set a discardable attribute.
  mlirOperationSetDiscardableAttributeByName(
      operation, mlirStringRefCreateFromCString("custom_attr"),
      mlirBoolAttrGet(ctx, 1));
  fprintf(stderr, "Op with set attr: ");
  mlirOperationPrint(operation, printToStderr, NULL);
  fprintf(stderr, "\n");
  // CHECK: Op with set attr: {{.*}} {custom_attr = true}

  // Remove the attribute.
  fprintf(stderr, "Remove attr: %d\n",
          mlirOperationRemoveDiscardableAttributeByName(
              operation, mlirStringRefCreateFromCString("custom_attr")));
  fprintf(stderr, "Remove attr again: %d\n",
          mlirOperationRemoveDiscardableAttributeByName(
              operation, mlirStringRefCreateFromCString("custom_attr")));
  fprintf(stderr, "Removed attr is null: %d\n",
          mlirAttributeIsNull(mlirOperationGetDiscardableAttributeByName(
              operation, mlirStringRefCreateFromCString("custom_attr"))));
  // CHECK: Remove attr: 1
  // CHECK: Remove attr again: 0
  // CHECK: Removed attr is null: 1

  // Add a large attribute to verify printing flags.
  int64_t eltsShape[] = {4};
  int32_t eltsData[] = {1, 2, 3, 4};
  mlirOperationSetDiscardableAttributeByName(
      operation, mlirStringRefCreateFromCString("elts"),
      mlirDenseElementsAttrInt32Get(
          mlirRankedTensorTypeGet(1, eltsShape, mlirIntegerTypeGet(ctx, 32),
                                  mlirAttributeGetNull()),
          4, eltsData));
  MlirOpPrintingFlags flags = mlirOpPrintingFlagsCreate();
  mlirOpPrintingFlagsElideLargeElementsAttrs(flags, 2);
  mlirOpPrintingFlagsPrintGenericOpForm(flags);
  mlirOpPrintingFlagsEnableDebugInfo(flags, /*enable=*/1, /*prettyForm=*/0);
  mlirOpPrintingFlagsUseLocalScope(flags);
  fprintf(stderr, "Op print with all flags: ");
  mlirOperationPrintWithFlags(operation, flags, printToStderr, NULL);
  fprintf(stderr, "\n");
  fprintf(stderr, "Op print with state: ");
  MlirAsmState state = mlirAsmStateCreateForOperation(parentOperation, flags);
  mlirOperationPrintWithState(operation, state, printToStderr, NULL);
  fprintf(stderr, "\n");
  // clang-format off
  // CHECK: Op print with all flags: %{{.*}} = "arith.constant"() <{value = 0 : index}> {elts = dense_resource<__elided__> : tensor<4xi32>} : () -> index loc(unknown)
  // clang-format on

  mlirOpPrintingFlagsDestroy(flags);
  flags = mlirOpPrintingFlagsCreate();
  mlirOpPrintingFlagsSkipRegions(flags);
  fprintf(stderr, "Op print with skip regions flag: ");
  mlirOperationPrintWithFlags(function, flags, printToStderr, NULL);
  fprintf(stderr, "\n");
  // clang-format off
  // CHECK: Op print with skip regions flag: func.func @add(%[[ARG0:.*]]: memref<?xf32>, %[[ARG1:.*]]: memref<?xf32>)
  // CHECK-NOT: constant
  // CHECK-NOT: return
  // clang-format on

  fprintf(stderr, "With state: |");
  mlirValuePrintAsOperand(value, state, printToStderr, NULL);
  // CHECK: With state: |%0|
  fprintf(stderr, "|\n");
  mlirAsmStateDestroy(state);

  mlirOpPrintingFlagsDestroy(flags);
}

static int constructAndTraverseIr(MlirContext ctx) {
  MlirLocation location = mlirLocationUnknownGet(ctx);

  MlirModule moduleOp = makeAndDumpAdd(ctx, location);
  MlirOperation module = mlirModuleGetOperation(moduleOp);
  assert(!mlirModuleIsNull(mlirModuleFromOperation(module)));

  int errcode = collectStats(module);
  if (errcode)
    return errcode;

  printFirstOfEach(ctx, module);

  mlirModuleDestroy(moduleOp);
  return 0;
}

/// Creates an operation with a region containing multiple blocks with
/// operations and dumps it. The blocks and operations are inserted using
/// block/operation-relative API and their final order is checked.
static void buildWithInsertionsAndPrint(MlirContext ctx) {
  MlirLocation loc = mlirLocationUnknownGet(ctx);
  mlirContextSetAllowUnregisteredDialects(ctx, true);

  MlirRegion owningRegion = mlirRegionCreate();
  MlirBlock nullBlock = mlirRegionGetFirstBlock(owningRegion);
  MlirOperationState state = mlirOperationStateGet(
      mlirStringRefCreateFromCString("insertion.order.test"), loc);
  mlirOperationStateAddOwnedRegions(&state, 1, &owningRegion);
  MlirOperation op = mlirOperationCreate(&state);
  MlirRegion region = mlirOperationGetRegion(op, 0);

  // Use integer types of different bitwidth as block arguments in order to
  // differentiate blocks.
  MlirType i1 = mlirIntegerTypeGet(ctx, 1);
  MlirType i2 = mlirIntegerTypeGet(ctx, 2);
  MlirType i3 = mlirIntegerTypeGet(ctx, 3);
  MlirType i4 = mlirIntegerTypeGet(ctx, 4);
  MlirType i5 = mlirIntegerTypeGet(ctx, 5);
  MlirBlock block1 = mlirBlockCreate(1, &i1, &loc);
  MlirBlock block2 = mlirBlockCreate(1, &i2, &loc);
  MlirBlock block3 = mlirBlockCreate(1, &i3, &loc);
  MlirBlock block4 = mlirBlockCreate(1, &i4, &loc);
  MlirBlock block5 = mlirBlockCreate(1, &i5, &loc);
  // Insert blocks so as to obtain the 1-2-3-4 order,
  mlirRegionInsertOwnedBlockBefore(region, nullBlock, block3);
  mlirRegionInsertOwnedBlockBefore(region, block3, block2);
  mlirRegionInsertOwnedBlockAfter(region, nullBlock, block1);
  mlirRegionInsertOwnedBlockAfter(region, block3, block4);
  mlirRegionInsertOwnedBlockBefore(region, block3, block5);

  MlirOperationState op1State =
      mlirOperationStateGet(mlirStringRefCreateFromCString("dummy.op1"), loc);
  MlirOperationState op2State =
      mlirOperationStateGet(mlirStringRefCreateFromCString("dummy.op2"), loc);
  MlirOperationState op3State =
      mlirOperationStateGet(mlirStringRefCreateFromCString("dummy.op3"), loc);
  MlirOperationState op4State =
      mlirOperationStateGet(mlirStringRefCreateFromCString("dummy.op4"), loc);
  MlirOperationState op5State =
      mlirOperationStateGet(mlirStringRefCreateFromCString("dummy.op5"), loc);
  MlirOperationState op6State =
      mlirOperationStateGet(mlirStringRefCreateFromCString("dummy.op6"), loc);
  MlirOperationState op7State =
      mlirOperationStateGet(mlirStringRefCreateFromCString("dummy.op7"), loc);
  MlirOperationState op8State =
      mlirOperationStateGet(mlirStringRefCreateFromCString("dummy.op8"), loc);
  MlirOperation op1 = mlirOperationCreate(&op1State);
  MlirOperation op2 = mlirOperationCreate(&op2State);
  MlirOperation op3 = mlirOperationCreate(&op3State);
  MlirOperation op4 = mlirOperationCreate(&op4State);
  MlirOperation op5 = mlirOperationCreate(&op5State);
  MlirOperation op6 = mlirOperationCreate(&op6State);
  MlirOperation op7 = mlirOperationCreate(&op7State);
  MlirOperation op8 = mlirOperationCreate(&op8State);

  // Insert operations in the first block so as to obtain the 1-2-3-4 order.
  MlirOperation nullOperation = mlirBlockGetFirstOperation(block1);
  assert(mlirOperationIsNull(nullOperation));
  mlirBlockInsertOwnedOperationBefore(block1, nullOperation, op3);
  mlirBlockInsertOwnedOperationBefore(block1, op3, op2);
  mlirBlockInsertOwnedOperationAfter(block1, nullOperation, op1);
  mlirBlockInsertOwnedOperationAfter(block1, op3, op4);

  // Append operations to the rest of blocks to make them non-empty and thus
  // printable.
  mlirBlockAppendOwnedOperation(block2, op5);
  mlirBlockAppendOwnedOperation(block3, op6);
  mlirBlockAppendOwnedOperation(block4, op7);
  mlirBlockAppendOwnedOperation(block5, op8);

  // Remove block5.
  mlirBlockDetach(block5);
  mlirBlockDestroy(block5);

  mlirOperationDump(op);
  mlirOperationDestroy(op);
  mlirContextSetAllowUnregisteredDialects(ctx, false);
  // clang-format off
  // CHECK-LABEL:  "insertion.order.test"
  // CHECK:      ^{{.*}}(%{{.*}}: i1
  // CHECK:        "dummy.op1"
  // CHECK-NEXT:   "dummy.op2"
  // CHECK-NEXT:   "dummy.op3"
  // CHECK-NEXT:   "dummy.op4"
  // CHECK:      ^{{.*}}(%{{.*}}: i2
  // CHECK:        "dummy.op5"
  // CHECK-NOT:  ^{{.*}}(%{{.*}}: i5
  // CHECK-NOT:    "dummy.op8"
  // CHECK:      ^{{.*}}(%{{.*}}: i3
  // CHECK:        "dummy.op6"
  // CHECK:      ^{{.*}}(%{{.*}}: i4
  // CHECK:        "dummy.op7"
  // clang-format on
}

/// Creates operations with type inference and tests various failure modes.
static int createOperationWithTypeInference(MlirContext ctx) {
  MlirLocation loc = mlirLocationUnknownGet(ctx);
  MlirAttribute iAttr = mlirIntegerAttrGet(mlirIntegerTypeGet(ctx, 32), 4);

  // The shape.const_size op implements result type inference and is only used
  // for that reason.
  MlirOperationState state = mlirOperationStateGet(
      mlirStringRefCreateFromCString("shape.const_size"), loc);
  MlirNamedAttribute valueAttr = mlirNamedAttributeGet(
      mlirIdentifierGet(ctx, mlirStringRefCreateFromCString("value")), iAttr);
  mlirOperationStateAddAttributes(&state, 1, &valueAttr);
  mlirOperationStateEnableResultTypeInference(&state);

  // Expect result type inference to succeed.
  MlirOperation op = mlirOperationCreate(&state);
  if (mlirOperationIsNull(op)) {
    fprintf(stderr, "ERROR: Result type inference unexpectedly failed");
    return 1;
  }

  // CHECK: RESULT_TYPE_INFERENCE: !shape.size
  fprintf(stderr, "RESULT_TYPE_INFERENCE: ");
  mlirTypeDump(mlirValueGetType(mlirOperationGetResult(op, 0)));
  fprintf(stderr, "\n");
  mlirOperationDestroy(op);
  return 0;
}

/// Dumps instances of all builtin types to check that C API works correctly.
/// Additionally, performs simple identity checks that a builtin type
/// constructed with C API can be inspected and has the expected type. The
/// latter achieves full coverage of C API for builtin types. Returns 0 on
/// success and a non-zero error code on failure.
static int printBuiltinTypes(MlirContext ctx) {
  // Integer types.
  MlirType i32 = mlirIntegerTypeGet(ctx, 32);
  MlirType si32 = mlirIntegerTypeSignedGet(ctx, 32);
  MlirType ui32 = mlirIntegerTypeUnsignedGet(ctx, 32);
  if (!mlirTypeIsAInteger(i32) || mlirTypeIsAF32(i32))
    return 1;
  if (!mlirTypeIsAInteger(si32) || !mlirIntegerTypeIsSigned(si32))
    return 2;
  if (!mlirTypeIsAInteger(ui32) || !mlirIntegerTypeIsUnsigned(ui32))
    return 3;
  if (mlirTypeEqual(i32, ui32) || mlirTypeEqual(i32, si32))
    return 4;
  if (mlirIntegerTypeGetWidth(i32) != mlirIntegerTypeGetWidth(si32))
    return 5;
  fprintf(stderr, "@types\n");
  mlirTypeDump(i32);
  fprintf(stderr, "\n");
  mlirTypeDump(si32);
  fprintf(stderr, "\n");
  mlirTypeDump(ui32);
  fprintf(stderr, "\n");
  // CHECK-LABEL: @types
  // CHECK: i32
  // CHECK: si32
  // CHECK: ui32

  // Index type.
  MlirType index = mlirIndexTypeGet(ctx);
  if (!mlirTypeIsAIndex(index))
    return 6;
  mlirTypeDump(index);
  fprintf(stderr, "\n");
  // CHECK: index

  // Floating-point types.
  MlirType bf16 = mlirBF16TypeGet(ctx);
  MlirType f16 = mlirF16TypeGet(ctx);
  MlirType f32 = mlirF32TypeGet(ctx);
  MlirType f64 = mlirF64TypeGet(ctx);
  if (!mlirTypeIsABF16(bf16))
    return 7;
  if (!mlirTypeIsAF16(f16))
    return 9;
  if (!mlirTypeIsAF32(f32))
    return 10;
  if (!mlirTypeIsAF64(f64))
    return 11;
  mlirTypeDump(bf16);
  fprintf(stderr, "\n");
  mlirTypeDump(f16);
  fprintf(stderr, "\n");
  mlirTypeDump(f32);
  fprintf(stderr, "\n");
  mlirTypeDump(f64);
  fprintf(stderr, "\n");
  // CHECK: bf16
  // CHECK: f16
  // CHECK: f32
  // CHECK: f64

  // None type.
  MlirType none = mlirNoneTypeGet(ctx);
  if (!mlirTypeIsANone(none))
    return 12;
  mlirTypeDump(none);
  fprintf(stderr, "\n");
  // CHECK: none

  // Complex type.
  MlirType cplx = mlirComplexTypeGet(f32);
  if (!mlirTypeIsAComplex(cplx) ||
      !mlirTypeEqual(mlirComplexTypeGetElementType(cplx), f32))
    return 13;
  mlirTypeDump(cplx);
  fprintf(stderr, "\n");
  // CHECK: complex<f32>

  // Vector (and Shaped) type. ShapedType is a common base class for vectors,
  // memrefs and tensors, one cannot create instances of this class so it is
  // tested on an instance of vector type.
  int64_t shape[] = {2, 3};
  MlirType vector =
      mlirVectorTypeGet(sizeof(shape) / sizeof(int64_t), shape, f32);
  if (!mlirTypeIsAVector(vector) || !mlirTypeIsAShaped(vector))
    return 14;
  if (!mlirTypeEqual(mlirShapedTypeGetElementType(vector), f32) ||
      !mlirShapedTypeHasRank(vector) || mlirShapedTypeGetRank(vector) != 2 ||
      mlirShapedTypeGetDimSize(vector, 0) != 2 ||
      mlirShapedTypeIsDynamicDim(vector, 0) ||
      mlirShapedTypeGetDimSize(vector, 1) != 3 ||
      !mlirShapedTypeHasStaticShape(vector))
    return 15;
  mlirTypeDump(vector);
  fprintf(stderr, "\n");
  // CHECK: vector<2x3xf32>

  // Scalable vector type.
  bool scalable[] = {false, true};
  MlirType scalableVector = mlirVectorTypeGetScalable(
      sizeof(shape) / sizeof(int64_t), shape, scalable, f32);
  if (!mlirTypeIsAVector(scalableVector))
    return 16;
  if (!mlirVectorTypeIsScalable(scalableVector) ||
      mlirVectorTypeIsDimScalable(scalableVector, 0) ||
      !mlirVectorTypeIsDimScalable(scalableVector, 1))
    return 17;
  mlirTypeDump(scalableVector);
  fprintf(stderr, "\n");
  // CHECK: vector<2x[3]xf32>

  // Ranked tensor type.
  MlirType rankedTensor = mlirRankedTensorTypeGet(
      sizeof(shape) / sizeof(int64_t), shape, f32, mlirAttributeGetNull());
  if (!mlirTypeIsATensor(rankedTensor) ||
      !mlirTypeIsARankedTensor(rankedTensor) ||
      !mlirAttributeIsNull(mlirRankedTensorTypeGetEncoding(rankedTensor)))
    return 18;
  mlirTypeDump(rankedTensor);
  fprintf(stderr, "\n");
  // CHECK: tensor<2x3xf32>

  // Unranked tensor type.
  MlirType unrankedTensor = mlirUnrankedTensorTypeGet(f32);
  if (!mlirTypeIsATensor(unrankedTensor) ||
      !mlirTypeIsAUnrankedTensor(unrankedTensor) ||
      mlirShapedTypeHasRank(unrankedTensor))
    return 19;
  mlirTypeDump(unrankedTensor);
  fprintf(stderr, "\n");
  // CHECK: tensor<*xf32>

  // MemRef type.
  MlirAttribute memSpace2 = mlirIntegerAttrGet(mlirIntegerTypeGet(ctx, 64), 2);
  MlirType memRef = mlirMemRefTypeContiguousGet(
      f32, sizeof(shape) / sizeof(int64_t), shape, memSpace2);
  if (!mlirTypeIsAMemRef(memRef) ||
      !mlirAttributeEqual(mlirMemRefTypeGetMemorySpace(memRef), memSpace2))
    return 20;
  mlirTypeDump(memRef);
  fprintf(stderr, "\n");
  // CHECK: memref<2x3xf32, 2>

  // Unranked MemRef type.
  MlirAttribute memSpace4 = mlirIntegerAttrGet(mlirIntegerTypeGet(ctx, 64), 4);
  MlirType unrankedMemRef = mlirUnrankedMemRefTypeGet(f32, memSpace4);
  if (!mlirTypeIsAUnrankedMemRef(unrankedMemRef) ||
      mlirTypeIsAMemRef(unrankedMemRef) ||
      !mlirAttributeEqual(mlirUnrankedMemrefGetMemorySpace(unrankedMemRef),
                          memSpace4))
    return 21;
  mlirTypeDump(unrankedMemRef);
  fprintf(stderr, "\n");
  // CHECK: memref<*xf32, 4>

  // Tuple type.
  MlirType types[] = {unrankedMemRef, f32};
  MlirType tuple = mlirTupleTypeGet(ctx, 2, types);
  if (!mlirTypeIsATuple(tuple) || mlirTupleTypeGetNumTypes(tuple) != 2 ||
      !mlirTypeEqual(mlirTupleTypeGetType(tuple, 0), unrankedMemRef) ||
      !mlirTypeEqual(mlirTupleTypeGetType(tuple, 1), f32))
    return 22;
  mlirTypeDump(tuple);
  fprintf(stderr, "\n");
  // CHECK: tuple<memref<*xf32, 4>, f32>

  // Function type.
  MlirType funcInputs[2] = {mlirIndexTypeGet(ctx), mlirIntegerTypeGet(ctx, 1)};
  MlirType funcResults[3] = {mlirIntegerTypeGet(ctx, 16),
                             mlirIntegerTypeGet(ctx, 32),
                             mlirIntegerTypeGet(ctx, 64)};
  MlirType funcType = mlirFunctionTypeGet(ctx, 2, funcInputs, 3, funcResults);
  if (mlirFunctionTypeGetNumInputs(funcType) != 2)
    return 23;
  if (mlirFunctionTypeGetNumResults(funcType) != 3)
    return 24;
  if (!mlirTypeEqual(funcInputs[0], mlirFunctionTypeGetInput(funcType, 0)) ||
      !mlirTypeEqual(funcInputs[1], mlirFunctionTypeGetInput(funcType, 1)))
    return 25;
  if (!mlirTypeEqual(funcResults[0], mlirFunctionTypeGetResult(funcType, 0)) ||
      !mlirTypeEqual(funcResults[1], mlirFunctionTypeGetResult(funcType, 1)) ||
      !mlirTypeEqual(funcResults[2], mlirFunctionTypeGetResult(funcType, 2)))
    return 26;
  mlirTypeDump(funcType);
  fprintf(stderr, "\n");
  // CHECK: (index, i1) -> (i16, i32, i64)

  // Opaque type.
  MlirStringRef namespace = mlirStringRefCreate("dialect", 7);
  MlirStringRef data = mlirStringRefCreate("type", 4);
  mlirContextSetAllowUnregisteredDialects(ctx, true);
  MlirType opaque = mlirOpaqueTypeGet(ctx, namespace, data);
  mlirContextSetAllowUnregisteredDialects(ctx, false);
  if (!mlirTypeIsAOpaque(opaque) ||
      !mlirStringRefEqual(mlirOpaqueTypeGetDialectNamespace(opaque),
                          namespace) ||
      !mlirStringRefEqual(mlirOpaqueTypeGetData(opaque), data))
    return 27;
  mlirTypeDump(opaque);
  fprintf(stderr, "\n");
  // CHECK: !dialect.type

  return 0;
}

void callbackSetFixedLengthString(const char *data, intptr_t len,
                                  void *userData) {
  strncpy(userData, data, len);
}

bool stringIsEqual(const char *lhs, MlirStringRef rhs) {
  if (strlen(lhs) != rhs.length) {
    return false;
  }
  return !strncmp(lhs, rhs.data, rhs.length);
}

int printBuiltinAttributes(MlirContext ctx) {
  MlirAttribute floating =
      mlirFloatAttrDoubleGet(ctx, mlirF64TypeGet(ctx), 2.0);
  if (!mlirAttributeIsAFloat(floating) ||
      fabs(mlirFloatAttrGetValueDouble(floating) - 2.0) > 1E-6)
    return 1;
  fprintf(stderr, "@attrs\n");
  mlirAttributeDump(floating);
  // CHECK-LABEL: @attrs
  // CHECK: 2.000000e+00 : f64

  // Exercise mlirAttributeGetType() just for the first one.
  MlirType floatingType = mlirAttributeGetType(floating);
  mlirTypeDump(floatingType);
  // CHECK: f64

  MlirAttribute integer = mlirIntegerAttrGet(mlirIntegerTypeGet(ctx, 32), 42);
  MlirAttribute signedInteger =
      mlirIntegerAttrGet(mlirIntegerTypeSignedGet(ctx, 8), -1);
  MlirAttribute unsignedInteger =
      mlirIntegerAttrGet(mlirIntegerTypeUnsignedGet(ctx, 8), 255);
  if (!mlirAttributeIsAInteger(integer) ||
      mlirIntegerAttrGetValueInt(integer) != 42 ||
      mlirIntegerAttrGetValueSInt(signedInteger) != -1 ||
      mlirIntegerAttrGetValueUInt(unsignedInteger) != 255)
    return 2;
  mlirAttributeDump(integer);
  mlirAttributeDump(signedInteger);
  mlirAttributeDump(unsignedInteger);
  // CHECK: 42 : i32
  // CHECK: -1 : si8
  // CHECK: 255 : ui8

  MlirAttribute boolean = mlirBoolAttrGet(ctx, 1);
  if (!mlirAttributeIsABool(boolean) || !mlirBoolAttrGetValue(boolean))
    return 3;
  mlirAttributeDump(boolean);
  // CHECK: true

  const char data[] = "abcdefghijklmnopqestuvwxyz";
  MlirAttribute opaque =
      mlirOpaqueAttrGet(ctx, mlirStringRefCreateFromCString("func"), 3, data,
                        mlirNoneTypeGet(ctx));
  if (!mlirAttributeIsAOpaque(opaque) ||
      !stringIsEqual("func", mlirOpaqueAttrGetDialectNamespace(opaque)))
    return 4;

  MlirStringRef opaqueData = mlirOpaqueAttrGetData(opaque);
  if (opaqueData.length != 3 ||
      strncmp(data, opaqueData.data, opaqueData.length))
    return 5;
  mlirAttributeDump(opaque);
  // CHECK: #func.abc

  MlirAttribute string =
      mlirStringAttrGet(ctx, mlirStringRefCreate(data + 3, 2));
  if (!mlirAttributeIsAString(string))
    return 6;

  MlirStringRef stringValue = mlirStringAttrGetValue(string);
  if (stringValue.length != 2 ||
      strncmp(data + 3, stringValue.data, stringValue.length))
    return 7;
  mlirAttributeDump(string);
  // CHECK: "de"

  MlirAttribute flatSymbolRef =
      mlirFlatSymbolRefAttrGet(ctx, mlirStringRefCreate(data + 5, 3));
  if (!mlirAttributeIsAFlatSymbolRef(flatSymbolRef))
    return 8;

  MlirStringRef flatSymbolRefValue =
      mlirFlatSymbolRefAttrGetValue(flatSymbolRef);
  if (flatSymbolRefValue.length != 3 ||
      strncmp(data + 5, flatSymbolRefValue.data, flatSymbolRefValue.length))
    return 9;
  mlirAttributeDump(flatSymbolRef);
  // CHECK: @fgh

  MlirAttribute symbols[] = {flatSymbolRef, flatSymbolRef};
  MlirAttribute symbolRef =
      mlirSymbolRefAttrGet(ctx, mlirStringRefCreate(data + 8, 2), 2, symbols);
  if (!mlirAttributeIsASymbolRef(symbolRef) ||
      mlirSymbolRefAttrGetNumNestedReferences(symbolRef) != 2 ||
      !mlirAttributeEqual(mlirSymbolRefAttrGetNestedReference(symbolRef, 0),
                          flatSymbolRef) ||
      !mlirAttributeEqual(mlirSymbolRefAttrGetNestedReference(symbolRef, 1),
                          flatSymbolRef))
    return 10;

  MlirStringRef symbolRefLeaf = mlirSymbolRefAttrGetLeafReference(symbolRef);
  MlirStringRef symbolRefRoot = mlirSymbolRefAttrGetRootReference(symbolRef);
  if (symbolRefLeaf.length != 3 ||
      strncmp(data + 5, symbolRefLeaf.data, symbolRefLeaf.length) ||
      symbolRefRoot.length != 2 ||
      strncmp(data + 8, symbolRefRoot.data, symbolRefRoot.length))
    return 11;
  mlirAttributeDump(symbolRef);
  // CHECK: @ij::@fgh::@fgh

  MlirAttribute type = mlirTypeAttrGet(mlirF32TypeGet(ctx));
  if (!mlirAttributeIsAType(type) ||
      !mlirTypeEqual(mlirF32TypeGet(ctx), mlirTypeAttrGetValue(type)))
    return 12;
  mlirAttributeDump(type);
  // CHECK: f32

  MlirAttribute unit = mlirUnitAttrGet(ctx);
  if (!mlirAttributeIsAUnit(unit))
    return 13;
  mlirAttributeDump(unit);
  // CHECK: unit

  int64_t shape[] = {1, 2};

  int bools[] = {0, 1};
  uint8_t uints8[] = {0u, 1u};
  int8_t ints8[] = {0, 1};
  uint16_t uints16[] = {0u, 1u};
  int16_t ints16[] = {0, 1};
  uint32_t uints32[] = {0u, 1u};
  int32_t ints32[] = {0, 1};
  uint64_t uints64[] = {0u, 1u};
  int64_t ints64[] = {0, 1};
  float floats[] = {0.0f, 1.0f};
  double doubles[] = {0.0, 1.0};
  uint16_t bf16s[] = {0x0, 0x3f80};
  uint16_t f16s[] = {0x0, 0x3c00};
  MlirAttribute encoding = mlirAttributeGetNull();
  MlirAttribute boolElements = mlirDenseElementsAttrBoolGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 1), encoding),
      2, bools);
  MlirAttribute uint8Elements = mlirDenseElementsAttrUInt8Get(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeUnsignedGet(ctx, 8),
                              encoding),
      2, uints8);
  MlirAttribute int8Elements = mlirDenseElementsAttrInt8Get(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 8), encoding),
      2, ints8);
  MlirAttribute uint16Elements = mlirDenseElementsAttrUInt16Get(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeUnsignedGet(ctx, 16),
                              encoding),
      2, uints16);
  MlirAttribute int16Elements = mlirDenseElementsAttrInt16Get(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 16), encoding),
      2, ints16);
  MlirAttribute uint32Elements = mlirDenseElementsAttrUInt32Get(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeUnsignedGet(ctx, 32),
                              encoding),
      2, uints32);
  MlirAttribute int32Elements = mlirDenseElementsAttrInt32Get(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 32), encoding),
      2, ints32);
  MlirAttribute uint64Elements = mlirDenseElementsAttrUInt64Get(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeUnsignedGet(ctx, 64),
                              encoding),
      2, uints64);
  MlirAttribute int64Elements = mlirDenseElementsAttrInt64Get(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 64), encoding),
      2, ints64);
  MlirAttribute floatElements = mlirDenseElementsAttrFloatGet(
      mlirRankedTensorTypeGet(2, shape, mlirF32TypeGet(ctx), encoding), 2,
      floats);
  MlirAttribute doubleElements = mlirDenseElementsAttrDoubleGet(
      mlirRankedTensorTypeGet(2, shape, mlirF64TypeGet(ctx), encoding), 2,
      doubles);
  MlirAttribute bf16Elements = mlirDenseElementsAttrBFloat16Get(
      mlirRankedTensorTypeGet(2, shape, mlirBF16TypeGet(ctx), encoding), 2,
      bf16s);
  MlirAttribute f16Elements = mlirDenseElementsAttrFloat16Get(
      mlirRankedTensorTypeGet(2, shape, mlirF16TypeGet(ctx), encoding), 2,
      f16s);

  if (!mlirAttributeIsADenseElements(boolElements) ||
      !mlirAttributeIsADenseElements(uint8Elements) ||
      !mlirAttributeIsADenseElements(int8Elements) ||
      !mlirAttributeIsADenseElements(uint32Elements) ||
      !mlirAttributeIsADenseElements(int32Elements) ||
      !mlirAttributeIsADenseElements(uint64Elements) ||
      !mlirAttributeIsADenseElements(int64Elements) ||
      !mlirAttributeIsADenseElements(floatElements) ||
      !mlirAttributeIsADenseElements(doubleElements) ||
      !mlirAttributeIsADenseElements(bf16Elements) ||
      !mlirAttributeIsADenseElements(f16Elements))
    return 14;

  if (mlirDenseElementsAttrGetBoolValue(boolElements, 1) != 1 ||
      mlirDenseElementsAttrGetUInt8Value(uint8Elements, 1) != 1 ||
      mlirDenseElementsAttrGetInt8Value(int8Elements, 1) != 1 ||
      mlirDenseElementsAttrGetUInt16Value(uint16Elements, 1) != 1 ||
      mlirDenseElementsAttrGetInt16Value(int16Elements, 1) != 1 ||
      mlirDenseElementsAttrGetUInt32Value(uint32Elements, 1) != 1 ||
      mlirDenseElementsAttrGetInt32Value(int32Elements, 1) != 1 ||
      mlirDenseElementsAttrGetUInt64Value(uint64Elements, 1) != 1 ||
      mlirDenseElementsAttrGetInt64Value(int64Elements, 1) != 1 ||
      fabsf(mlirDenseElementsAttrGetFloatValue(floatElements, 1) - 1.0f) >
          1E-6f ||
      fabs(mlirDenseElementsAttrGetDoubleValue(doubleElements, 1) - 1.0) > 1E-6)
    return 15;

  mlirAttributeDump(boolElements);
  mlirAttributeDump(uint8Elements);
  mlirAttributeDump(int8Elements);
  mlirAttributeDump(uint32Elements);
  mlirAttributeDump(int32Elements);
  mlirAttributeDump(uint64Elements);
  mlirAttributeDump(int64Elements);
  mlirAttributeDump(floatElements);
  mlirAttributeDump(doubleElements);
  mlirAttributeDump(bf16Elements);
  mlirAttributeDump(f16Elements);
  // CHECK: dense<{{\[}}[false, true]]> : tensor<1x2xi1>
  // CHECK: dense<{{\[}}[0, 1]]> : tensor<1x2xui8>
  // CHECK: dense<{{\[}}[0, 1]]> : tensor<1x2xi8>
  // CHECK: dense<{{\[}}[0, 1]]> : tensor<1x2xui32>
  // CHECK: dense<{{\[}}[0, 1]]> : tensor<1x2xi32>
  // CHECK: dense<{{\[}}[0, 1]]> : tensor<1x2xui64>
  // CHECK: dense<{{\[}}[0, 1]]> : tensor<1x2xi64>
  // CHECK: dense<{{\[}}[0.000000e+00, 1.000000e+00]]> : tensor<1x2xf32>
  // CHECK: dense<{{\[}}[0.000000e+00, 1.000000e+00]]> : tensor<1x2xf64>
  // CHECK: dense<{{\[}}[0.000000e+00, 1.000000e+00]]> : tensor<1x2xbf16>
  // CHECK: dense<{{\[}}[0.000000e+00, 1.000000e+00]]> : tensor<1x2xf16>

  MlirAttribute splatBool = mlirDenseElementsAttrBoolSplatGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 1), encoding),
      1);
  MlirAttribute splatUInt8 = mlirDenseElementsAttrUInt8SplatGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeUnsignedGet(ctx, 8),
                              encoding),
      1);
  MlirAttribute splatInt8 = mlirDenseElementsAttrInt8SplatGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 8), encoding),
      1);
  MlirAttribute splatUInt32 = mlirDenseElementsAttrUInt32SplatGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeUnsignedGet(ctx, 32),
                              encoding),
      1);
  MlirAttribute splatInt32 = mlirDenseElementsAttrInt32SplatGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 32), encoding),
      1);
  MlirAttribute splatUInt64 = mlirDenseElementsAttrUInt64SplatGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeUnsignedGet(ctx, 64),
                              encoding),
      1);
  MlirAttribute splatInt64 = mlirDenseElementsAttrInt64SplatGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 64), encoding),
      1);
  MlirAttribute splatFloat = mlirDenseElementsAttrFloatSplatGet(
      mlirRankedTensorTypeGet(2, shape, mlirF32TypeGet(ctx), encoding), 1.0f);
  MlirAttribute splatDouble = mlirDenseElementsAttrDoubleSplatGet(
      mlirRankedTensorTypeGet(2, shape, mlirF64TypeGet(ctx), encoding), 1.0);

  if (!mlirAttributeIsADenseElements(splatBool) ||
      !mlirDenseElementsAttrIsSplat(splatBool) ||
      !mlirAttributeIsADenseElements(splatUInt8) ||
      !mlirDenseElementsAttrIsSplat(splatUInt8) ||
      !mlirAttributeIsADenseElements(splatInt8) ||
      !mlirDenseElementsAttrIsSplat(splatInt8) ||
      !mlirAttributeIsADenseElements(splatUInt32) ||
      !mlirDenseElementsAttrIsSplat(splatUInt32) ||
      !mlirAttributeIsADenseElements(splatInt32) ||
      !mlirDenseElementsAttrIsSplat(splatInt32) ||
      !mlirAttributeIsADenseElements(splatUInt64) ||
      !mlirDenseElementsAttrIsSplat(splatUInt64) ||
      !mlirAttributeIsADenseElements(splatInt64) ||
      !mlirDenseElementsAttrIsSplat(splatInt64) ||
      !mlirAttributeIsADenseElements(splatFloat) ||
      !mlirDenseElementsAttrIsSplat(splatFloat) ||
      !mlirAttributeIsADenseElements(splatDouble) ||
      !mlirDenseElementsAttrIsSplat(splatDouble))
    return 16;

  if (mlirDenseElementsAttrGetBoolSplatValue(splatBool) != 1 ||
      mlirDenseElementsAttrGetUInt8SplatValue(splatUInt8) != 1 ||
      mlirDenseElementsAttrGetInt8SplatValue(splatInt8) != 1 ||
      mlirDenseElementsAttrGetUInt32SplatValue(splatUInt32) != 1 ||
      mlirDenseElementsAttrGetInt32SplatValue(splatInt32) != 1 ||
      mlirDenseElementsAttrGetUInt64SplatValue(splatUInt64) != 1 ||
      mlirDenseElementsAttrGetInt64SplatValue(splatInt64) != 1 ||
      fabsf(mlirDenseElementsAttrGetFloatSplatValue(splatFloat) - 1.0f) >
          1E-6f ||
      fabs(mlirDenseElementsAttrGetDoubleSplatValue(splatDouble) - 1.0) > 1E-6)
    return 17;

  const uint8_t *uint8RawData =
      (const uint8_t *)mlirDenseElementsAttrGetRawData(uint8Elements);
  const int8_t *int8RawData =
      (const int8_t *)mlirDenseElementsAttrGetRawData(int8Elements);
  const uint32_t *uint32RawData =
      (const uint32_t *)mlirDenseElementsAttrGetRawData(uint32Elements);
  const int32_t *int32RawData =
      (const int32_t *)mlirDenseElementsAttrGetRawData(int32Elements);
  const uint64_t *uint64RawData =
      (const uint64_t *)mlirDenseElementsAttrGetRawData(uint64Elements);
  const int64_t *int64RawData =
      (const int64_t *)mlirDenseElementsAttrGetRawData(int64Elements);
  const float *floatRawData =
      (const float *)mlirDenseElementsAttrGetRawData(floatElements);
  const double *doubleRawData =
      (const double *)mlirDenseElementsAttrGetRawData(doubleElements);
  const uint16_t *bf16RawData =
      (const uint16_t *)mlirDenseElementsAttrGetRawData(bf16Elements);
  const uint16_t *f16RawData =
      (const uint16_t *)mlirDenseElementsAttrGetRawData(f16Elements);
  if (uint8RawData[0] != 0u || uint8RawData[1] != 1u || int8RawData[0] != 0 ||
      int8RawData[1] != 1 || uint32RawData[0] != 0u || uint32RawData[1] != 1u ||
      int32RawData[0] != 0 || int32RawData[1] != 1 || uint64RawData[0] != 0u ||
      uint64RawData[1] != 1u || int64RawData[0] != 0 || int64RawData[1] != 1 ||
      floatRawData[0] != 0.0f || floatRawData[1] != 1.0f ||
      doubleRawData[0] != 0.0 || doubleRawData[1] != 1.0 ||
      bf16RawData[0] != 0 || bf16RawData[1] != 0x3f80 || f16RawData[0] != 0 ||
      f16RawData[1] != 0x3c00)
    return 18;

  mlirAttributeDump(splatBool);
  mlirAttributeDump(splatUInt8);
  mlirAttributeDump(splatInt8);
  mlirAttributeDump(splatUInt32);
  mlirAttributeDump(splatInt32);
  mlirAttributeDump(splatUInt64);
  mlirAttributeDump(splatInt64);
  mlirAttributeDump(splatFloat);
  mlirAttributeDump(splatDouble);
  // CHECK: dense<true> : tensor<1x2xi1>
  // CHECK: dense<1> : tensor<1x2xui8>
  // CHECK: dense<1> : tensor<1x2xi8>
  // CHECK: dense<1> : tensor<1x2xui32>
  // CHECK: dense<1> : tensor<1x2xi32>
  // CHECK: dense<1> : tensor<1x2xui64>
  // CHECK: dense<1> : tensor<1x2xi64>
  // CHECK: dense<1.000000e+00> : tensor<1x2xf32>
  // CHECK: dense<1.000000e+00> : tensor<1x2xf64>

  mlirAttributeDump(mlirElementsAttrGetValue(floatElements, 2, uints64));
  mlirAttributeDump(mlirElementsAttrGetValue(doubleElements, 2, uints64));
  mlirAttributeDump(mlirElementsAttrGetValue(bf16Elements, 2, uints64));
  mlirAttributeDump(mlirElementsAttrGetValue(f16Elements, 2, uints64));
  // CHECK: 1.000000e+00 : f32
  // CHECK: 1.000000e+00 : f64
  // CHECK: 1.000000e+00 : bf16
  // CHECK: 1.000000e+00 : f16

  int64_t indices[] = {0, 1};
  int64_t one = 1;
  MlirAttribute indicesAttr = mlirDenseElementsAttrInt64Get(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 64), encoding),
      2, indices);
  MlirAttribute valuesAttr = mlirDenseElementsAttrFloatGet(
      mlirRankedTensorTypeGet(1, &one, mlirF32TypeGet(ctx), encoding), 1,
      floats);
  MlirAttribute sparseAttr = mlirSparseElementsAttribute(
      mlirRankedTensorTypeGet(2, shape, mlirF32TypeGet(ctx), encoding),
      indicesAttr, valuesAttr);
  mlirAttributeDump(sparseAttr);
  // CHECK: sparse<{{\[}}[0, 1]], 0.000000e+00> : tensor<1x2xf32>

  MlirAttribute boolArray = mlirDenseBoolArrayGet(ctx, 2, bools);
  MlirAttribute int8Array = mlirDenseI8ArrayGet(ctx, 2, ints8);
  MlirAttribute int16Array = mlirDenseI16ArrayGet(ctx, 2, ints16);
  MlirAttribute int32Array = mlirDenseI32ArrayGet(ctx, 2, ints32);
  MlirAttribute int64Array = mlirDenseI64ArrayGet(ctx, 2, ints64);
  MlirAttribute floatArray = mlirDenseF32ArrayGet(ctx, 2, floats);
  MlirAttribute doubleArray = mlirDenseF64ArrayGet(ctx, 2, doubles);
  if (!mlirAttributeIsADenseBoolArray(boolArray) ||
      !mlirAttributeIsADenseI8Array(int8Array) ||
      !mlirAttributeIsADenseI16Array(int16Array) ||
      !mlirAttributeIsADenseI32Array(int32Array) ||
      !mlirAttributeIsADenseI64Array(int64Array) ||
      !mlirAttributeIsADenseF32Array(floatArray) ||
      !mlirAttributeIsADenseF64Array(doubleArray))
    return 19;

  if (mlirDenseArrayGetNumElements(boolArray) != 2 ||
      mlirDenseArrayGetNumElements(int8Array) != 2 ||
      mlirDenseArrayGetNumElements(int16Array) != 2 ||
      mlirDenseArrayGetNumElements(int32Array) != 2 ||
      mlirDenseArrayGetNumElements(int64Array) != 2 ||
      mlirDenseArrayGetNumElements(floatArray) != 2 ||
      mlirDenseArrayGetNumElements(doubleArray) != 2)
    return 20;

  if (mlirDenseBoolArrayGetElement(boolArray, 1) != 1 ||
      mlirDenseI8ArrayGetElement(int8Array, 1) != 1 ||
      mlirDenseI16ArrayGetElement(int16Array, 1) != 1 ||
      mlirDenseI32ArrayGetElement(int32Array, 1) != 1 ||
      mlirDenseI64ArrayGetElement(int64Array, 1) != 1 ||
      fabsf(mlirDenseF32ArrayGetElement(floatArray, 1) - 1.0f) > 1E-6f ||
      fabs(mlirDenseF64ArrayGetElement(doubleArray, 1) - 1.0) > 1E-6)
    return 21;

  int64_t layoutStrides[3] = {5, 7, 13};
  MlirAttribute stridedLayoutAttr =
      mlirStridedLayoutAttrGet(ctx, 42, 3, &layoutStrides[0]);

  // CHECK: strided<[5, 7, 13], offset: 42>
  mlirAttributeDump(stridedLayoutAttr);

  if (mlirStridedLayoutAttrGetOffset(stridedLayoutAttr) != 42 ||
      mlirStridedLayoutAttrGetNumStrides(stridedLayoutAttr) != 3 ||
      mlirStridedLayoutAttrGetStride(stridedLayoutAttr, 0) != 5 ||
      mlirStridedLayoutAttrGetStride(stridedLayoutAttr, 1) != 7 ||
      mlirStridedLayoutAttrGetStride(stridedLayoutAttr, 2) != 13)
    return 22;

  MlirAttribute uint8Blob = mlirUnmanagedDenseUInt8ResourceElementsAttrGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeUnsignedGet(ctx, 8),
                              encoding),
      mlirStringRefCreateFromCString("resource_ui8"), 2, uints8);
  MlirAttribute uint16Blob = mlirUnmanagedDenseUInt16ResourceElementsAttrGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeUnsignedGet(ctx, 16),
                              encoding),
      mlirStringRefCreateFromCString("resource_ui16"), 2, uints16);
  MlirAttribute uint32Blob = mlirUnmanagedDenseUInt32ResourceElementsAttrGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeUnsignedGet(ctx, 32),
                              encoding),
      mlirStringRefCreateFromCString("resource_ui32"), 2, uints32);
  MlirAttribute uint64Blob = mlirUnmanagedDenseUInt64ResourceElementsAttrGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeUnsignedGet(ctx, 64),
                              encoding),
      mlirStringRefCreateFromCString("resource_ui64"), 2, uints64);
  MlirAttribute int8Blob = mlirUnmanagedDenseInt8ResourceElementsAttrGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 8), encoding),
      mlirStringRefCreateFromCString("resource_i8"), 2, ints8);
  MlirAttribute int16Blob = mlirUnmanagedDenseInt16ResourceElementsAttrGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 16), encoding),
      mlirStringRefCreateFromCString("resource_i16"), 2, ints16);
  MlirAttribute int32Blob = mlirUnmanagedDenseInt32ResourceElementsAttrGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 32), encoding),
      mlirStringRefCreateFromCString("resource_i32"), 2, ints32);
  MlirAttribute int64Blob = mlirUnmanagedDenseInt64ResourceElementsAttrGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 64), encoding),
      mlirStringRefCreateFromCString("resource_i64"), 2, ints64);
  MlirAttribute floatsBlob = mlirUnmanagedDenseFloatResourceElementsAttrGet(
      mlirRankedTensorTypeGet(2, shape, mlirF32TypeGet(ctx), encoding),
      mlirStringRefCreateFromCString("resource_f32"), 2, floats);
  MlirAttribute doublesBlob = mlirUnmanagedDenseDoubleResourceElementsAttrGet(
      mlirRankedTensorTypeGet(2, shape, mlirF64TypeGet(ctx), encoding),
      mlirStringRefCreateFromCString("resource_f64"), 2, doubles);
  MlirAttribute blobBlob = mlirUnmanagedDenseResourceElementsAttrGet(
      mlirRankedTensorTypeGet(2, shape, mlirIntegerTypeGet(ctx, 64), encoding),
      mlirStringRefCreateFromCString("resource_i64_blob"), /*data=*/uints64,
      /*dataLength=*/sizeof(uints64),
      /*dataAlignment=*/_Alignof(uint64_t),
      /*dataIsMutable=*/false,
      /*deleter=*/reportResourceDelete,
      /*userData=*/(void *)&resourceI64BlobUserData);

  mlirAttributeDump(uint8Blob);
  mlirAttributeDump(uint16Blob);
  mlirAttributeDump(uint32Blob);
  mlirAttributeDump(uint64Blob);
  mlirAttributeDump(int8Blob);
  mlirAttributeDump(int16Blob);
  mlirAttributeDump(int32Blob);
  mlirAttributeDump(int64Blob);
  mlirAttributeDump(floatsBlob);
  mlirAttributeDump(doublesBlob);
  mlirAttributeDump(blobBlob);
  // CHECK: dense_resource<resource_ui8> : tensor<1x2xui8>
  // CHECK: dense_resource<resource_ui16> : tensor<1x2xui16>
  // CHECK: dense_resource<resource_ui32> : tensor<1x2xui32>
  // CHECK: dense_resource<resource_ui64> : tensor<1x2xui64>
  // CHECK: dense_resource<resource_i8> : tensor<1x2xi8>
  // CHECK: dense_resource<resource_i16> : tensor<1x2xi16>
  // CHECK: dense_resource<resource_i32> : tensor<1x2xi32>
  // CHECK: dense_resource<resource_i64> : tensor<1x2xi64>
  // CHECK: dense_resource<resource_f32> : tensor<1x2xf32>
  // CHECK: dense_resource<resource_f64> : tensor<1x2xf64>
  // CHECK: dense_resource<resource_i64_blob> : tensor<1x2xi64>

  if (mlirDenseUInt8ResourceElementsAttrGetValue(uint8Blob, 1) != 1 ||
      mlirDenseUInt16ResourceElementsAttrGetValue(uint16Blob, 1) != 1 ||
      mlirDenseUInt32ResourceElementsAttrGetValue(uint32Blob, 1) != 1 ||
      mlirDenseUInt64ResourceElementsAttrGetValue(uint64Blob, 1) != 1 ||
      mlirDenseInt8ResourceElementsAttrGetValue(int8Blob, 1) != 1 ||
      mlirDenseInt16ResourceElementsAttrGetValue(int16Blob, 1) != 1 ||
      mlirDenseInt32ResourceElementsAttrGetValue(int32Blob, 1) != 1 ||
      mlirDenseInt64ResourceElementsAttrGetValue(int64Blob, 1) != 1 ||
      fabsf(mlirDenseF32ArrayGetElement(floatArray, 1) - 1.0f) > 1E-6f ||
      fabsf(mlirDenseFloatResourceElementsAttrGetValue(floatsBlob, 1) - 1.0f) >
          1e-6 ||
      fabs(mlirDenseDoubleResourceElementsAttrGetValue(doublesBlob, 1) - 1.0f) >
          1e-6 ||
      mlirDenseUInt64ResourceElementsAttrGetValue(blobBlob, 1) != 1)
    return 23;

  MlirLocation loc = mlirLocationUnknownGet(ctx);
  MlirAttribute locAttr = mlirLocationGetAttribute(loc);
  if (!mlirAttributeIsALocation(locAttr))
    return 24;

  return 0;
}

int printAffineMap(MlirContext ctx) {
  MlirAffineMap emptyAffineMap = mlirAffineMapEmptyGet(ctx);
  MlirAffineMap affineMap = mlirAffineMapZeroResultGet(ctx, 3, 2);
  MlirAffineMap constAffineMap = mlirAffineMapConstantGet(ctx, 2);
  MlirAffineMap multiDimIdentityAffineMap =
      mlirAffineMapMultiDimIdentityGet(ctx, 3);
  MlirAffineMap minorIdentityAffineMap =
      mlirAffineMapMinorIdentityGet(ctx, 3, 2);
  unsigned permutation[] = {1, 2, 0};
  MlirAffineMap permutationAffineMap = mlirAffineMapPermutationGet(
      ctx, sizeof(permutation) / sizeof(unsigned), permutation);

  fprintf(stderr, "@affineMap\n");
  mlirAffineMapDump(emptyAffineMap);
  mlirAffineMapDump(affineMap);
  mlirAffineMapDump(constAffineMap);
  mlirAffineMapDump(multiDimIdentityAffineMap);
  mlirAffineMapDump(minorIdentityAffineMap);
  mlirAffineMapDump(permutationAffineMap);
  // CHECK-LABEL: @affineMap
  // CHECK: () -> ()
  // CHECK: (d0, d1, d2)[s0, s1] -> ()
  // CHECK: () -> (2)
  // CHECK: (d0, d1, d2) -> (d0, d1, d2)
  // CHECK: (d0, d1, d2) -> (d1, d2)
  // CHECK: (d0, d1, d2) -> (d1, d2, d0)

  if (!mlirAffineMapIsIdentity(emptyAffineMap) ||
      mlirAffineMapIsIdentity(affineMap) ||
      mlirAffineMapIsIdentity(constAffineMap) ||
      !mlirAffineMapIsIdentity(multiDimIdentityAffineMap) ||
      mlirAffineMapIsIdentity(minorIdentityAffineMap) ||
      mlirAffineMapIsIdentity(permutationAffineMap))
    return 1;

  if (!mlirAffineMapIsMinorIdentity(emptyAffineMap) ||
      mlirAffineMapIsMinorIdentity(affineMap) ||
      !mlirAffineMapIsMinorIdentity(multiDimIdentityAffineMap) ||
      !mlirAffineMapIsMinorIdentity(minorIdentityAffineMap) ||
      mlirAffineMapIsMinorIdentity(permutationAffineMap))
    return 2;

  if (!mlirAffineMapIsEmpty(emptyAffineMap) ||
      mlirAffineMapIsEmpty(affineMap) || mlirAffineMapIsEmpty(constAffineMap) ||
      mlirAffineMapIsEmpty(multiDimIdentityAffineMap) ||
      mlirAffineMapIsEmpty(minorIdentityAffineMap) ||
      mlirAffineMapIsEmpty(permutationAffineMap))
    return 3;

  if (mlirAffineMapIsSingleConstant(emptyAffineMap) ||
      mlirAffineMapIsSingleConstant(affineMap) ||
      !mlirAffineMapIsSingleConstant(constAffineMap) ||
      mlirAffineMapIsSingleConstant(multiDimIdentityAffineMap) ||
      mlirAffineMapIsSingleConstant(minorIdentityAffineMap) ||
      mlirAffineMapIsSingleConstant(permutationAffineMap))
    return 4;

  if (mlirAffineMapGetSingleConstantResult(constAffineMap) != 2)
    return 5;

  if (mlirAffineMapGetNumDims(emptyAffineMap) != 0 ||
      mlirAffineMapGetNumDims(affineMap) != 3 ||
      mlirAffineMapGetNumDims(constAffineMap) != 0 ||
      mlirAffineMapGetNumDims(multiDimIdentityAffineMap) != 3 ||
      mlirAffineMapGetNumDims(minorIdentityAffineMap) != 3 ||
      mlirAffineMapGetNumDims(permutationAffineMap) != 3)
    return 6;

  if (mlirAffineMapGetNumSymbols(emptyAffineMap) != 0 ||
      mlirAffineMapGetNumSymbols(affineMap) != 2 ||
      mlirAffineMapGetNumSymbols(constAffineMap) != 0 ||
      mlirAffineMapGetNumSymbols(multiDimIdentityAffineMap) != 0 ||
      mlirAffineMapGetNumSymbols(minorIdentityAffineMap) != 0 ||
      mlirAffineMapGetNumSymbols(permutationAffineMap) != 0)
    return 7;

  if (mlirAffineMapGetNumResults(emptyAffineMap) != 0 ||
      mlirAffineMapGetNumResults(affineMap) != 0 ||
      mlirAffineMapGetNumResults(constAffineMap) != 1 ||
      mlirAffineMapGetNumResults(multiDimIdentityAffineMap) != 3 ||
      mlirAffineMapGetNumResults(minorIdentityAffineMap) != 2 ||
      mlirAffineMapGetNumResults(permutationAffineMap) != 3)
    return 8;

  if (mlirAffineMapGetNumInputs(emptyAffineMap) != 0 ||
      mlirAffineMapGetNumInputs(affineMap) != 5 ||
      mlirAffineMapGetNumInputs(constAffineMap) != 0 ||
      mlirAffineMapGetNumInputs(multiDimIdentityAffineMap) != 3 ||
      mlirAffineMapGetNumInputs(minorIdentityAffineMap) != 3 ||
      mlirAffineMapGetNumInputs(permutationAffineMap) != 3)
    return 9;

  if (!mlirAffineMapIsProjectedPermutation(emptyAffineMap) ||
      !mlirAffineMapIsPermutation(emptyAffineMap) ||
      mlirAffineMapIsProjectedPermutation(affineMap) ||
      mlirAffineMapIsPermutation(affineMap) ||
      mlirAffineMapIsProjectedPermutation(constAffineMap) ||
      mlirAffineMapIsPermutation(constAffineMap) ||
      !mlirAffineMapIsProjectedPermutation(multiDimIdentityAffineMap) ||
      !mlirAffineMapIsPermutation(multiDimIdentityAffineMap) ||
      !mlirAffineMapIsProjectedPermutation(minorIdentityAffineMap) ||
      mlirAffineMapIsPermutation(minorIdentityAffineMap) ||
      !mlirAffineMapIsProjectedPermutation(permutationAffineMap) ||
      !mlirAffineMapIsPermutation(permutationAffineMap))
    return 10;

  intptr_t sub[] = {1};

  MlirAffineMap subMap = mlirAffineMapGetSubMap(
      multiDimIdentityAffineMap, sizeof(sub) / sizeof(intptr_t), sub);
  MlirAffineMap majorSubMap =
      mlirAffineMapGetMajorSubMap(multiDimIdentityAffineMap, 1);
  MlirAffineMap minorSubMap =
      mlirAffineMapGetMinorSubMap(multiDimIdentityAffineMap, 1);

  mlirAffineMapDump(subMap);
  mlirAffineMapDump(majorSubMap);
  mlirAffineMapDump(minorSubMap);
  // CHECK: (d0, d1, d2) -> (d1)
  // CHECK: (d0, d1, d2) -> (d0)
  // CHECK: (d0, d1, d2) -> (d2)

  // CHECK: distinct[0]<"foo">
  mlirAttributeDump(mlirDisctinctAttrCreate(
      mlirStringAttrGet(ctx, mlirStringRefCreateFromCString("foo"))));

  return 0;
}

int printAffineExpr(MlirContext ctx) {
  MlirAffineExpr affineDimExpr = mlirAffineDimExprGet(ctx, 5);
  MlirAffineExpr affineSymbolExpr = mlirAffineSymbolExprGet(ctx, 5);
  MlirAffineExpr affineConstantExpr = mlirAffineConstantExprGet(ctx, 5);
  MlirAffineExpr affineAddExpr =
      mlirAffineAddExprGet(affineDimExpr, affineSymbolExpr);
  MlirAffineExpr affineMulExpr =
      mlirAffineMulExprGet(affineDimExpr, affineSymbolExpr);
  MlirAffineExpr affineModExpr =
      mlirAffineModExprGet(affineDimExpr, affineSymbolExpr);
  MlirAffineExpr affineFloorDivExpr =
      mlirAffineFloorDivExprGet(affineDimExpr, affineSymbolExpr);
  MlirAffineExpr affineCeilDivExpr =
      mlirAffineCeilDivExprGet(affineDimExpr, affineSymbolExpr);

  // Tests mlirAffineExprDump.
  fprintf(stderr, "@affineExpr\n");
  mlirAffineExprDump(affineDimExpr);
  mlirAffineExprDump(affineSymbolExpr);
  mlirAffineExprDump(affineConstantExpr);
  mlirAffineExprDump(affineAddExpr);
  mlirAffineExprDump(affineMulExpr);
  mlirAffineExprDump(affineModExpr);
  mlirAffineExprDump(affineFloorDivExpr);
  mlirAffineExprDump(affineCeilDivExpr);
  // CHECK-LABEL: @affineExpr
  // CHECK: d5
  // CHECK: s5
  // CHECK: 5
  // CHECK: d5 + s5
  // CHECK: d5 * s5
  // CHECK: d5 mod s5
  // CHECK: d5 floordiv s5
  // CHECK: d5 ceildiv s5

  // Tests methods of affine binary operation expression, takes add expression
  // as an example.
  mlirAffineExprDump(mlirAffineBinaryOpExprGetLHS(affineAddExpr));
  mlirAffineExprDump(mlirAffineBinaryOpExprGetRHS(affineAddExpr));
  // CHECK: d5
  // CHECK: s5

  // Tests methods of affine dimension expression.
  if (mlirAffineDimExprGetPosition(affineDimExpr) != 5)
    return 1;

  // Tests methods of affine symbol expression.
  if (mlirAffineSymbolExprGetPosition(affineSymbolExpr) != 5)
    return 2;

  // Tests methods of affine constant expression.
  if (mlirAffineConstantExprGetValue(affineConstantExpr) != 5)
    return 3;

  // Tests methods of affine expression.
  if (mlirAffineExprIsSymbolicOrConstant(affineDimExpr) ||
      !mlirAffineExprIsSymbolicOrConstant(affineSymbolExpr) ||
      !mlirAffineExprIsSymbolicOrConstant(affineConstantExpr) ||
      mlirAffineExprIsSymbolicOrConstant(affineAddExpr) ||
      mlirAffineExprIsSymbolicOrConstant(affineMulExpr) ||
      mlirAffineExprIsSymbolicOrConstant(affineModExpr) ||
      mlirAffineExprIsSymbolicOrConstant(affineFloorDivExpr) ||
      mlirAffineExprIsSymbolicOrConstant(affineCeilDivExpr))
    return 4;

  if (!mlirAffineExprIsPureAffine(affineDimExpr) ||
      !mlirAffineExprIsPureAffine(affineSymbolExpr) ||
      !mlirAffineExprIsPureAffine(affineConstantExpr) ||
      !mlirAffineExprIsPureAffine(affineAddExpr) ||
      mlirAffineExprIsPureAffine(affineMulExpr) ||
      mlirAffineExprIsPureAffine(affineModExpr) ||
      mlirAffineExprIsPureAffine(affineFloorDivExpr) ||
      mlirAffineExprIsPureAffine(affineCeilDivExpr))
    return 5;

  if (mlirAffineExprGetLargestKnownDivisor(affineDimExpr) != 1 ||
      mlirAffineExprGetLargestKnownDivisor(affineSymbolExpr) != 1 ||
      mlirAffineExprGetLargestKnownDivisor(affineConstantExpr) != 5 ||
      mlirAffineExprGetLargestKnownDivisor(affineAddExpr) != 1 ||
      mlirAffineExprGetLargestKnownDivisor(affineMulExpr) != 1 ||
      mlirAffineExprGetLargestKnownDivisor(affineModExpr) != 1 ||
      mlirAffineExprGetLargestKnownDivisor(affineFloorDivExpr) != 1 ||
      mlirAffineExprGetLargestKnownDivisor(affineCeilDivExpr) != 1)
    return 6;

  if (!mlirAffineExprIsMultipleOf(affineDimExpr, 1) ||
      !mlirAffineExprIsMultipleOf(affineSymbolExpr, 1) ||
      !mlirAffineExprIsMultipleOf(affineConstantExpr, 5) ||
      !mlirAffineExprIsMultipleOf(affineAddExpr, 1) ||
      !mlirAffineExprIsMultipleOf(affineMulExpr, 1) ||
      !mlirAffineExprIsMultipleOf(affineModExpr, 1) ||
      !mlirAffineExprIsMultipleOf(affineFloorDivExpr, 1) ||
      !mlirAffineExprIsMultipleOf(affineCeilDivExpr, 1))
    return 7;

  if (!mlirAffineExprIsFunctionOfDim(affineDimExpr, 5) ||
      mlirAffineExprIsFunctionOfDim(affineSymbolExpr, 5) ||
      mlirAffineExprIsFunctionOfDim(affineConstantExpr, 5) ||
      !mlirAffineExprIsFunctionOfDim(affineAddExpr, 5) ||
      !mlirAffineExprIsFunctionOfDim(affineMulExpr, 5) ||
      !mlirAffineExprIsFunctionOfDim(affineModExpr, 5) ||
      !mlirAffineExprIsFunctionOfDim(affineFloorDivExpr, 5) ||
      !mlirAffineExprIsFunctionOfDim(affineCeilDivExpr, 5))
    return 8;

  // Tests 'IsA' methods of affine binary operation expression.
  if (!mlirAffineExprIsAAdd(affineAddExpr))
    return 9;

  if (!mlirAffineExprIsAMul(affineMulExpr))
    return 10;

  if (!mlirAffineExprIsAMod(affineModExpr))
    return 11;

  if (!mlirAffineExprIsAFloorDiv(affineFloorDivExpr))
    return 12;

  if (!mlirAffineExprIsACeilDiv(affineCeilDivExpr))
    return 13;

  if (!mlirAffineExprIsABinary(affineAddExpr))
    return 14;

  // Test other 'IsA' method on affine expressions.
  if (!mlirAffineExprIsAConstant(affineConstantExpr))
    return 15;

  if (!mlirAffineExprIsADim(affineDimExpr))
    return 16;

  if (!mlirAffineExprIsASymbol(affineSymbolExpr))
    return 17;

  // Test equality and nullity.
  MlirAffineExpr otherDimExpr = mlirAffineDimExprGet(ctx, 5);
  if (!mlirAffineExprEqual(affineDimExpr, otherDimExpr))
    return 18;

  if (mlirAffineExprIsNull(affineDimExpr))
    return 19;

  return 0;
}

int affineMapFromExprs(MlirContext ctx) {
  MlirAffineExpr affineDimExpr = mlirAffineDimExprGet(ctx, 0);
  MlirAffineExpr affineSymbolExpr = mlirAffineSymbolExprGet(ctx, 1);
  MlirAffineExpr exprs[] = {affineDimExpr, affineSymbolExpr};
  MlirAffineMap map = mlirAffineMapGet(ctx, 3, 3, 2, exprs);

  // CHECK-LABEL: @affineMapFromExprs
  fprintf(stderr, "@affineMapFromExprs");
  // CHECK: (d0, d1, d2)[s0, s1, s2] -> (d0, s1)
  mlirAffineMapDump(map);

  if (mlirAffineMapGetNumResults(map) != 2)
    return 1;

  if (!mlirAffineExprEqual(mlirAffineMapGetResult(map, 0), affineDimExpr))
    return 2;

  if (!mlirAffineExprEqual(mlirAffineMapGetResult(map, 1), affineSymbolExpr))
    return 3;

  MlirAffineExpr affineDim2Expr = mlirAffineDimExprGet(ctx, 1);
  MlirAffineExpr composed = mlirAffineExprCompose(affineDim2Expr, map);
  // CHECK: s1
  mlirAffineExprDump(composed);
  if (!mlirAffineExprEqual(composed, affineSymbolExpr))
    return 4;

  return 0;
}

int printIntegerSet(MlirContext ctx) {
  MlirIntegerSet emptySet = mlirIntegerSetEmptyGet(ctx, 2, 1);

  // CHECK-LABEL: @printIntegerSet
  fprintf(stderr, "@printIntegerSet");

  // CHECK: (d0, d1)[s0] : (1 == 0)
  mlirIntegerSetDump(emptySet);

  if (!mlirIntegerSetIsCanonicalEmpty(emptySet))
    return 1;

  MlirIntegerSet anotherEmptySet = mlirIntegerSetEmptyGet(ctx, 2, 1);
  if (!mlirIntegerSetEqual(emptySet, anotherEmptySet))
    return 2;

  // Construct a set constrained by:
  //   d0 - s0 == 0,
  //   d1 - 42 >= 0.
  MlirAffineExpr negOne = mlirAffineConstantExprGet(ctx, -1);
  MlirAffineExpr negFortyTwo = mlirAffineConstantExprGet(ctx, -42);
  MlirAffineExpr d0 = mlirAffineDimExprGet(ctx, 0);
  MlirAffineExpr d1 = mlirAffineDimExprGet(ctx, 1);
  MlirAffineExpr s0 = mlirAffineSymbolExprGet(ctx, 0);
  MlirAffineExpr negS0 = mlirAffineMulExprGet(negOne, s0);
  MlirAffineExpr d0minusS0 = mlirAffineAddExprGet(d0, negS0);
  MlirAffineExpr d1minus42 = mlirAffineAddExprGet(d1, negFortyTwo);
  MlirAffineExpr constraints[] = {d0minusS0, d1minus42};
  bool flags[] = {true, false};

  MlirIntegerSet set = mlirIntegerSetGet(ctx, 2, 1, 2, constraints, flags);
  // CHECK: (d0, d1)[s0] : (
  // CHECK-DAG: d0 - s0 == 0
  // CHECK-DAG: d1 - 42 >= 0
  mlirIntegerSetDump(set);

  // Transform d1 into s0.
  MlirAffineExpr s1 = mlirAffineSymbolExprGet(ctx, 1);
  MlirAffineExpr repl[] = {d0, s1};
  MlirIntegerSet replaced = mlirIntegerSetReplaceGet(set, repl, &s0, 1, 2);
  // CHECK: (d0)[s0, s1] : (
  // CHECK-DAG: d0 - s0 == 0
  // CHECK-DAG: s1 - 42 >= 0
  mlirIntegerSetDump(replaced);

  if (mlirIntegerSetGetNumDims(set) != 2)
    return 3;
  if (mlirIntegerSetGetNumDims(replaced) != 1)
    return 4;

  if (mlirIntegerSetGetNumSymbols(set) != 1)
    return 5;
  if (mlirIntegerSetGetNumSymbols(replaced) != 2)
    return 6;

  if (mlirIntegerSetGetNumInputs(set) != 3)
    return 7;

  if (mlirIntegerSetGetNumConstraints(set) != 2)
    return 8;

  if (mlirIntegerSetGetNumEqualities(set) != 1)
    return 9;

  if (mlirIntegerSetGetNumInequalities(set) != 1)
    return 10;

  MlirAffineExpr cstr1 = mlirIntegerSetGetConstraint(set, 0);
  MlirAffineExpr cstr2 = mlirIntegerSetGetConstraint(set, 1);
  bool isEq1 = mlirIntegerSetIsConstraintEq(set, 0);
  bool isEq2 = mlirIntegerSetIsConstraintEq(set, 1);
  if (!mlirAffineExprEqual(cstr1, isEq1 ? d0minusS0 : d1minus42))
    return 11;
  if (!mlirAffineExprEqual(cstr2, isEq2 ? d0minusS0 : d1minus42))
    return 12;

  return 0;
}

int registerOnlyStd(void) {
  MlirContext ctx = mlirContextCreate();
  // The built-in dialect is always loaded.
  if (mlirContextGetNumLoadedDialects(ctx) != 1)
    return 1;

  MlirDialectHandle stdHandle = mlirGetDialectHandle__func__();

  MlirDialect std = mlirContextGetOrLoadDialect(
      ctx, mlirDialectHandleGetNamespace(stdHandle));
  if (!mlirDialectIsNull(std))
    return 2;

  mlirDialectHandleRegisterDialect(stdHandle, ctx);

  std = mlirContextGetOrLoadDialect(ctx,
                                    mlirDialectHandleGetNamespace(stdHandle));
  if (mlirDialectIsNull(std))
    return 3;

  MlirDialect alsoStd = mlirDialectHandleLoadDialect(stdHandle, ctx);
  if (!mlirDialectEqual(std, alsoStd))
    return 4;

  MlirStringRef stdNs = mlirDialectGetNamespace(std);
  MlirStringRef alsoStdNs = mlirDialectHandleGetNamespace(stdHandle);
  if (stdNs.length != alsoStdNs.length ||
      strncmp(stdNs.data, alsoStdNs.data, stdNs.length))
    return 5;

  fprintf(stderr, "@registration\n");
  // CHECK-LABEL: @registration

  // CHECK: func.call is_registered: 1
  fprintf(stderr, "func.call is_registered: %d\n",
          mlirContextIsRegisteredOperation(
              ctx, mlirStringRefCreateFromCString("func.call")));

  // CHECK: func.not_existing_op is_registered: 0
  fprintf(stderr, "func.not_existing_op is_registered: %d\n",
          mlirContextIsRegisteredOperation(
              ctx, mlirStringRefCreateFromCString("func.not_existing_op")));

  // CHECK: not_existing_dialect.not_existing_op is_registered: 0
  fprintf(stderr, "not_existing_dialect.not_existing_op is_registered: %d\n",
          mlirContextIsRegisteredOperation(
              ctx, mlirStringRefCreateFromCString(
                       "not_existing_dialect.not_existing_op")));

  mlirContextDestroy(ctx);
  return 0;
}

/// Tests backreference APIs
static int testBackreferences(void) {
  fprintf(stderr, "@test_backreferences\n");

  MlirContext ctx = mlirContextCreate();
  mlirContextSetAllowUnregisteredDialects(ctx, true);
  MlirLocation loc = mlirLocationUnknownGet(ctx);

  MlirOperationState opState =
      mlirOperationStateGet(mlirStringRefCreateFromCString("invalid.op"), loc);
  MlirRegion region = mlirRegionCreate();
  MlirBlock block = mlirBlockCreate(0, NULL, NULL);
  mlirRegionAppendOwnedBlock(region, block);
  mlirOperationStateAddOwnedRegions(&opState, 1, &region);
  MlirOperation op = mlirOperationCreate(&opState);
  MlirIdentifier ident =
      mlirIdentifierGet(ctx, mlirStringRefCreateFromCString("identifier"));

  if (!mlirContextEqual(ctx, mlirOperationGetContext(op))) {
    fprintf(stderr, "ERROR: Getting context from operation failed\n");
    return 1;
  }
  if (!mlirOperationEqual(op, mlirBlockGetParentOperation(block))) {
    fprintf(stderr, "ERROR: Getting parent operation from block failed\n");
    return 2;
  }
  if (!mlirContextEqual(ctx, mlirIdentifierGetContext(ident))) {
    fprintf(stderr, "ERROR: Getting context from identifier failed\n");
    return 3;
  }

  mlirOperationDestroy(op);
  mlirContextDestroy(ctx);

  // CHECK-LABEL: @test_backreferences
  return 0;
}

/// Tests operand APIs.
int testOperands(void) {
  fprintf(stderr, "@testOperands\n");
  // CHECK-LABEL: @testOperands

  MlirContext ctx = mlirContextCreate();
  registerAllUpstreamDialects(ctx);

  mlirContextGetOrLoadDialect(ctx, mlirStringRefCreateFromCString("arith"));
  mlirContextGetOrLoadDialect(ctx, mlirStringRefCreateFromCString("test"));
  MlirLocation loc = mlirLocationUnknownGet(ctx);
  MlirType indexType = mlirIndexTypeGet(ctx);

  // Create some constants to use as operands.
  MlirAttribute indexZeroLiteral =
      mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("0 : index"));
  MlirNamedAttribute indexZeroValueAttr = mlirNamedAttributeGet(
      mlirIdentifierGet(ctx, mlirStringRefCreateFromCString("value")),
      indexZeroLiteral);
  MlirOperationState constZeroState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("arith.constant"), loc);
  mlirOperationStateAddResults(&constZeroState, 1, &indexType);
  mlirOperationStateAddAttributes(&constZeroState, 1, &indexZeroValueAttr);
  MlirOperation constZero = mlirOperationCreate(&constZeroState);
  MlirValue constZeroValue = mlirOperationGetResult(constZero, 0);

  MlirAttribute indexOneLiteral =
      mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("1 : index"));
  MlirNamedAttribute indexOneValueAttr = mlirNamedAttributeGet(
      mlirIdentifierGet(ctx, mlirStringRefCreateFromCString("value")),
      indexOneLiteral);
  MlirOperationState constOneState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("arith.constant"), loc);
  mlirOperationStateAddResults(&constOneState, 1, &indexType);
  mlirOperationStateAddAttributes(&constOneState, 1, &indexOneValueAttr);
  MlirOperation constOne = mlirOperationCreate(&constOneState);
  MlirValue constOneValue = mlirOperationGetResult(constOne, 0);

  // Create the operation under test.
  mlirContextSetAllowUnregisteredDialects(ctx, true);
  MlirOperationState opState =
      mlirOperationStateGet(mlirStringRefCreateFromCString("dummy.op"), loc);
  MlirValue initialOperands[] = {constZeroValue};
  mlirOperationStateAddOperands(&opState, 1, initialOperands);
  MlirOperation op = mlirOperationCreate(&opState);

  // Test operand APIs.
  intptr_t numOperands = mlirOperationGetNumOperands(op);
  fprintf(stderr, "Num Operands: %" PRIdPTR "\n", numOperands);
  // CHECK: Num Operands: 1

  MlirValue opOperand1 = mlirOperationGetOperand(op, 0);
  fprintf(stderr, "Original operand: ");
  mlirValuePrint(opOperand1, printToStderr, NULL);
  // CHECK: Original operand: {{.+}} arith.constant 0 : index

  mlirOperationSetOperand(op, 0, constOneValue);
  MlirValue opOperand2 = mlirOperationGetOperand(op, 0);
  fprintf(stderr, "Updated operand: ");
  mlirValuePrint(opOperand2, printToStderr, NULL);
  // CHECK: Updated operand: {{.+}} arith.constant 1 : index

  // Test op operand APIs.
  MlirOpOperand use1 = mlirValueGetFirstUse(opOperand1);
  if (!mlirOpOperandIsNull(use1)) {
    fprintf(stderr, "ERROR: Use should be null\n");
    return 1;
  }

  MlirOpOperand use2 = mlirValueGetFirstUse(opOperand2);
  if (mlirOpOperandIsNull(use2)) {
    fprintf(stderr, "ERROR: Use should not be null\n");
    return 2;
  }

  fprintf(stderr, "Use owner: ");
  mlirOperationPrint(mlirOpOperandGetOwner(use2), printToStderr, NULL);
  fprintf(stderr, "\n");
  // CHECK: Use owner: "dummy.op"

  fprintf(stderr, "Use operandNumber: %d\n",
          mlirOpOperandGetOperandNumber(use2));
  // CHECK: Use operandNumber: 0

  use2 = mlirOpOperandGetNextUse(use2);
  if (!mlirOpOperandIsNull(use2)) {
    fprintf(stderr, "ERROR: Next use should be null\n");
    return 3;
  }

  MlirOperationState op2State =
      mlirOperationStateGet(mlirStringRefCreateFromCString("dummy.op2"), loc);
  MlirValue initialOperands2[] = {constOneValue};
  mlirOperationStateAddOperands(&op2State, 1, initialOperands2);
  MlirOperation op2 = mlirOperationCreate(&op2State);

  MlirOpOperand use3 = mlirValueGetFirstUse(constOneValue);
  fprintf(stderr, "First use owner: ");
  mlirOperationPrint(mlirOpOperandGetOwner(use3), printToStderr, NULL);
  fprintf(stderr, "\n");
  // CHECK: First use owner: "dummy.op2"

  use3 = mlirOpOperandGetNextUse(mlirValueGetFirstUse(constOneValue));
  fprintf(stderr, "Second use owner: ");
  mlirOperationPrint(mlirOpOperandGetOwner(use3), printToStderr, NULL);
  fprintf(stderr, "\n");
  // CHECK: Second use owner: "dummy.op"

  MlirAttribute indexTwoLiteral =
      mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("2 : index"));
  MlirNamedAttribute indexTwoValueAttr = mlirNamedAttributeGet(
      mlirIdentifierGet(ctx, mlirStringRefCreateFromCString("value")),
      indexTwoLiteral);
  MlirOperationState constTwoState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("arith.constant"), loc);
  mlirOperationStateAddResults(&constTwoState, 1, &indexType);
  mlirOperationStateAddAttributes(&constTwoState, 1, &indexTwoValueAttr);
  MlirOperation constTwo = mlirOperationCreate(&constTwoState);
  MlirValue constTwoValue = mlirOperationGetResult(constTwo, 0);

  mlirValueReplaceAllUsesOfWith(constOneValue, constTwoValue);

  use3 = mlirValueGetFirstUse(constOneValue);
  if (!mlirOpOperandIsNull(use3)) {
    fprintf(stderr, "ERROR: Use should be null\n");
    return 4;
  }

  MlirOpOperand use4 = mlirValueGetFirstUse(constTwoValue);
  fprintf(stderr, "First replacement use owner: ");
  mlirOperationPrint(mlirOpOperandGetOwner(use4), printToStderr, NULL);
  fprintf(stderr, "\n");
  // CHECK: First replacement use owner: "dummy.op"

  use4 = mlirOpOperandGetNextUse(mlirValueGetFirstUse(constTwoValue));
  fprintf(stderr, "Second replacement use owner: ");
  mlirOperationPrint(mlirOpOperandGetOwner(use4), printToStderr, NULL);
  fprintf(stderr, "\n");
  // CHECK: Second replacement use owner: "dummy.op2"

  MlirOpOperand use5 = mlirValueGetFirstUse(constTwoValue);
  MlirOpOperand use6 = mlirOpOperandGetNextUse(use5);
  if (!mlirValueEqual(mlirOpOperandGetValue(use5),
                      mlirOpOperandGetValue(use6))) {
    fprintf(stderr,
            "ERROR: First and second operand should share the same value\n");
    return 5;
  }

  mlirOperationDestroy(op);
  mlirOperationDestroy(op2);
  mlirOperationDestroy(constZero);
  mlirOperationDestroy(constOne);
  mlirOperationDestroy(constTwo);
  mlirContextDestroy(ctx);

  return 0;
}

/// Tests clone APIs.
int testClone(void) {
  fprintf(stderr, "@testClone\n");
  // CHECK-LABEL: @testClone

  MlirContext ctx = mlirContextCreate();
  registerAllUpstreamDialects(ctx);

  mlirContextGetOrLoadDialect(ctx, mlirStringRefCreateFromCString("func"));
  mlirContextGetOrLoadDialect(ctx, mlirStringRefCreateFromCString("arith"));
  MlirLocation loc = mlirLocationUnknownGet(ctx);
  MlirType indexType = mlirIndexTypeGet(ctx);
  MlirStringRef valueStringRef = mlirStringRefCreateFromCString("value");

  MlirAttribute indexZeroLiteral =
      mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("0 : index"));
  MlirNamedAttribute indexZeroValueAttr = mlirNamedAttributeGet(
      mlirIdentifierGet(ctx, valueStringRef), indexZeroLiteral);
  MlirOperationState constZeroState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("arith.constant"), loc);
  mlirOperationStateAddResults(&constZeroState, 1, &indexType);
  mlirOperationStateAddAttributes(&constZeroState, 1, &indexZeroValueAttr);
  MlirOperation constZero = mlirOperationCreate(&constZeroState);

  MlirAttribute indexOneLiteral =
      mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("1 : index"));
  MlirOperation constOne = mlirOperationClone(constZero);
  mlirOperationSetAttributeByName(constOne, valueStringRef, indexOneLiteral);

  mlirOperationPrint(constZero, printToStderr, NULL);
  mlirOperationPrint(constOne, printToStderr, NULL);
  // CHECK: arith.constant 0 : index
  // CHECK: arith.constant 1 : index

  mlirOperationDestroy(constZero);
  mlirOperationDestroy(constOne);
  mlirContextDestroy(ctx);
  return 0;
}

// Wraps a diagnostic into additional text we can match against.
MlirLogicalResult errorHandler(MlirDiagnostic diagnostic, void *userData) {
  fprintf(stderr, "processing diagnostic (userData: %" PRIdPTR ") <<\n",
          (intptr_t)userData);
  mlirDiagnosticPrint(diagnostic, printToStderr, NULL);
  fprintf(stderr, "\n");
  MlirLocation loc = mlirDiagnosticGetLocation(diagnostic);
  mlirLocationPrint(loc, printToStderr, NULL);
  assert(mlirDiagnosticGetNumNotes(diagnostic) == 0);
  fprintf(stderr, "\n>> end of diagnostic (userData: %" PRIdPTR ")\n",
          (intptr_t)userData);
  return mlirLogicalResultSuccess();
}

// Logs when the delete user data callback is called
static void deleteUserData(void *userData) {
  fprintf(stderr, "deleting user data (userData: %" PRIdPTR ")\n",
          (intptr_t)userData);
}

int testTypeID(MlirContext ctx) {
  fprintf(stderr, "@testTypeID\n");

  // Test getting and comparing type and attribute type ids.
  MlirType i32 = mlirIntegerTypeGet(ctx, 32);
  MlirTypeID i32ID = mlirTypeGetTypeID(i32);
  MlirType ui32 = mlirIntegerTypeUnsignedGet(ctx, 32);
  MlirTypeID ui32ID = mlirTypeGetTypeID(ui32);
  MlirType f32 = mlirF32TypeGet(ctx);
  MlirTypeID f32ID = mlirTypeGetTypeID(f32);
  MlirAttribute i32Attr = mlirIntegerAttrGet(i32, 1);
  MlirTypeID i32AttrID = mlirAttributeGetTypeID(i32Attr);

  if (mlirTypeIDIsNull(i32ID) || mlirTypeIDIsNull(ui32ID) ||
      mlirTypeIDIsNull(f32ID) || mlirTypeIDIsNull(i32AttrID)) {
    fprintf(stderr, "ERROR: Expected type ids to be present\n");
    return 1;
  }

  if (!mlirTypeIDEqual(i32ID, ui32ID) ||
      mlirTypeIDHashValue(i32ID) != mlirTypeIDHashValue(ui32ID)) {
    fprintf(
        stderr,
        "ERROR: Expected different integer types to have the same type id\n");
    return 2;
  }

  if (mlirTypeIDEqual(i32ID, f32ID)) {
    fprintf(stderr,
            "ERROR: Expected integer type id to not equal float type id\n");
    return 3;
  }

  if (mlirTypeIDEqual(i32ID, i32AttrID)) {
    fprintf(stderr, "ERROR: Expected integer type id to not equal integer "
                    "attribute type id\n");
    return 4;
  }

  MlirLocation loc = mlirLocationUnknownGet(ctx);
  MlirType indexType = mlirIndexTypeGet(ctx);
  MlirStringRef valueStringRef = mlirStringRefCreateFromCString("value");

  // Create a registered operation, which should have a type id.
  MlirAttribute indexZeroLiteral =
      mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("0 : index"));
  MlirNamedAttribute indexZeroValueAttr = mlirNamedAttributeGet(
      mlirIdentifierGet(ctx, valueStringRef), indexZeroLiteral);
  MlirOperationState constZeroState = mlirOperationStateGet(
      mlirStringRefCreateFromCString("arith.constant"), loc);
  mlirOperationStateAddResults(&constZeroState, 1, &indexType);
  mlirOperationStateAddAttributes(&constZeroState, 1, &indexZeroValueAttr);
  MlirOperation constZero = mlirOperationCreate(&constZeroState);

  if (!mlirOperationVerify(constZero)) {
    fprintf(stderr, "ERROR: Expected operation to verify correctly\n");
    return 5;
  }

  if (mlirOperationIsNull(constZero)) {
    fprintf(stderr, "ERROR: Expected registered operation to be present\n");
    return 6;
  }

  MlirTypeID registeredOpID = mlirOperationGetTypeID(constZero);

  if (mlirTypeIDIsNull(registeredOpID)) {
    fprintf(stderr,
            "ERROR: Expected registered operation type id to be present\n");
    return 7;
  }

  // Create an unregistered operation, which should not have a type id.
  mlirContextSetAllowUnregisteredDialects(ctx, true);
  MlirOperationState opState =
      mlirOperationStateGet(mlirStringRefCreateFromCString("dummy.op"), loc);
  MlirOperation unregisteredOp = mlirOperationCreate(&opState);
  if (mlirOperationIsNull(unregisteredOp)) {
    fprintf(stderr, "ERROR: Expected unregistered operation to be present\n");
    return 8;
  }

  MlirTypeID unregisteredOpID = mlirOperationGetTypeID(unregisteredOp);

  if (!mlirTypeIDIsNull(unregisteredOpID)) {
    fprintf(stderr,
            "ERROR: Expected unregistered operation type id to be null\n");
    return 9;
  }

  mlirOperationDestroy(constZero);
  mlirOperationDestroy(unregisteredOp);

  return 0;
}

int testSymbolTable(MlirContext ctx) {
  fprintf(stderr, "@testSymbolTable\n");

  const char *moduleString = "func.func private @foo()"
                             "func.func private @bar()";
  const char *otherModuleString = "func.func private @qux()"
                                  "func.func private @foo()";

  MlirModule module =
      mlirModuleCreateParse(ctx, mlirStringRefCreateFromCString(moduleString));
  MlirModule otherModule = mlirModuleCreateParse(
      ctx, mlirStringRefCreateFromCString(otherModuleString));

  MlirSymbolTable symbolTable =
      mlirSymbolTableCreate(mlirModuleGetOperation(module));

  MlirOperation funcFoo =
      mlirSymbolTableLookup(symbolTable, mlirStringRefCreateFromCString("foo"));
  if (mlirOperationIsNull(funcFoo))
    return 1;

  MlirOperation funcBar =
      mlirSymbolTableLookup(symbolTable, mlirStringRefCreateFromCString("bar"));
  if (mlirOperationEqual(funcFoo, funcBar))
    return 2;

  MlirOperation missing =
      mlirSymbolTableLookup(symbolTable, mlirStringRefCreateFromCString("qux"));
  if (!mlirOperationIsNull(missing))
    return 3;

  MlirBlock moduleBody = mlirModuleGetBody(module);
  MlirBlock otherModuleBody = mlirModuleGetBody(otherModule);
  MlirOperation operation = mlirBlockGetFirstOperation(otherModuleBody);
  mlirOperationRemoveFromParent(operation);
  mlirBlockAppendOwnedOperation(moduleBody, operation);

  // At this moment, the operation is still missing from the symbol table.
  MlirOperation stillMissing =
      mlirSymbolTableLookup(symbolTable, mlirStringRefCreateFromCString("qux"));
  if (!mlirOperationIsNull(stillMissing))
    return 4;

  // After it is added to the symbol table, and not only the operation with
  // which the table is associated, it can be looked up.
  mlirSymbolTableInsert(symbolTable, operation);
  MlirOperation funcQux =
      mlirSymbolTableLookup(symbolTable, mlirStringRefCreateFromCString("qux"));
  if (!mlirOperationEqual(operation, funcQux))
    return 5;

  // Erasing from the symbol table also removes the operation.
  mlirSymbolTableErase(symbolTable, funcBar);
  MlirOperation nowMissing =
      mlirSymbolTableLookup(symbolTable, mlirStringRefCreateFromCString("bar"));
  if (!mlirOperationIsNull(nowMissing))
    return 6;

  // Adding a symbol with the same name to the table should rename.
  MlirOperation duplicateNameOp = mlirBlockGetFirstOperation(otherModuleBody);
  mlirOperationRemoveFromParent(duplicateNameOp);
  mlirBlockAppendOwnedOperation(moduleBody, duplicateNameOp);
  MlirAttribute newName = mlirSymbolTableInsert(symbolTable, duplicateNameOp);
  MlirStringRef newNameStr = mlirStringAttrGetValue(newName);
  if (mlirStringRefEqual(newNameStr, mlirStringRefCreateFromCString("foo")))
    return 7;
  MlirAttribute updatedName = mlirOperationGetAttributeByName(
      duplicateNameOp, mlirSymbolTableGetSymbolAttributeName());
  if (!mlirAttributeEqual(updatedName, newName))
    return 8;

  mlirOperationDump(mlirModuleGetOperation(module));
  mlirOperationDump(mlirModuleGetOperation(otherModule));
  // clang-format off
  // CHECK-LABEL: @testSymbolTable
  // CHECK: module
  // CHECK:   func private @foo
  // CHECK:   func private @qux
  // CHECK:   func private @foo{{.+}}
  // CHECK: module
  // CHECK-NOT: @qux
  // CHECK-NOT: @foo
  // clang-format on

  mlirSymbolTableDestroy(symbolTable);
  mlirModuleDestroy(module);
  mlirModuleDestroy(otherModule);

  return 0;
}

typedef struct {
  const char *x;
} callBackData;

MlirWalkResult walkCallBack(MlirOperation op, void *rootOpVoid) {
  fprintf(stderr, "%s: %s\n", ((callBackData *)(rootOpVoid))->x,
          mlirIdentifierStr(mlirOperationGetName(op)).data);
  return MlirWalkResultAdvance;
}

MlirWalkResult walkCallBackTestWalkResult(MlirOperation op, void *rootOpVoid) {
  fprintf(stderr, "%s: %s\n", ((callBackData *)(rootOpVoid))->x,
          mlirIdentifierStr(mlirOperationGetName(op)).data);
  if (strcmp(mlirIdentifierStr(mlirOperationGetName(op)).data, "func.func") ==
      0)
    return MlirWalkResultSkip;
  if (strcmp(mlirIdentifierStr(mlirOperationGetName(op)).data, "arith.addi") ==
      0)
    return MlirWalkResultInterrupt;
  return MlirWalkResultAdvance;
}

int testOperationWalk(MlirContext ctx) {
  // CHECK-LABEL: @testOperationWalk
  fprintf(stderr, "@testOperationWalk\n");

  const char *moduleString = "module {\n"
                             "  func.func @foo() {\n"
                             "    %1 = arith.constant 10: i32\n"
                             "    arith.addi %1, %1: i32\n"
                             "    return\n"
                             "  }\n"
                             "  func.func @bar() {\n"
                             "    return\n"
                             "  }\n"
                             "}";
  MlirModule module =
      mlirModuleCreateParse(ctx, mlirStringRefCreateFromCString(moduleString));

  callBackData data;
  data.x = "i love you";

  // CHECK-NEXT: i love you: arith.constant
  // CHECK-NEXT: i love you: arith.addi
  // CHECK-NEXT: i love you: func.return
  // CHECK-NEXT: i love you: func.func
  // CHECK-NEXT: i love you: func.return
  // CHECK-NEXT: i love you: func.func
  // CHECK-NEXT: i love you: builtin.module
  mlirOperationWalk(mlirModuleGetOperation(module), walkCallBack,
                    (void *)(&data), MlirWalkPostOrder);

  data.x = "i don't love you";
  // CHECK-NEXT: i don't love you: builtin.module
  // CHECK-NEXT: i don't love you: func.func
  // CHECK-NEXT: i don't love you: arith.constant
  // CHECK-NEXT: i don't love you: arith.addi
  // CHECK-NEXT: i don't love you: func.return
  // CHECK-NEXT: i don't love you: func.func
  // CHECK-NEXT: i don't love you: func.return
  mlirOperationWalk(mlirModuleGetOperation(module), walkCallBack,
                    (void *)(&data), MlirWalkPreOrder);

  data.x = "interrupt";
  // Interrupted at `arith.addi`
  // CHECK-NEXT: interrupt: arith.constant
  // CHECK-NEXT: interrupt: arith.addi
  mlirOperationWalk(mlirModuleGetOperation(module), walkCallBackTestWalkResult,
                    (void *)(&data), MlirWalkPostOrder);

  data.x = "skip";
  // Skip at `func.func`
  // CHECK-NEXT: skip: builtin.module
  // CHECK-NEXT: skip: func.func
  // CHECK-NEXT: skip: func.func
  mlirOperationWalk(mlirModuleGetOperation(module), walkCallBackTestWalkResult,
                    (void *)(&data), MlirWalkPreOrder);

  mlirModuleDestroy(module);
  return 0;
}

int testDialectRegistry(void) {
  fprintf(stderr, "@testDialectRegistry\n");

  MlirDialectRegistry registry = mlirDialectRegistryCreate();
  if (mlirDialectRegistryIsNull(registry)) {
    fprintf(stderr, "ERROR: Expected registry to be present\n");
    return 1;
  }

  MlirDialectHandle stdHandle = mlirGetDialectHandle__func__();
  mlirDialectHandleInsertDialect(stdHandle, registry);

  MlirContext ctx = mlirContextCreate();
  if (mlirContextGetNumRegisteredDialects(ctx) != 0) {
    fprintf(stderr,
            "ERROR: Expected no dialects to be registered to new context\n");
  }

  mlirContextAppendDialectRegistry(ctx, registry);
  if (mlirContextGetNumRegisteredDialects(ctx) != 1) {
    fprintf(stderr, "ERROR: Expected the dialect in the registry to be "
                    "registered to the context\n");
  }

  mlirContextDestroy(ctx);
  mlirDialectRegistryDestroy(registry);

  return 0;
}

void testExplicitThreadPools(void) {
  MlirLlvmThreadPool threadPool = mlirLlvmThreadPoolCreate();
  MlirDialectRegistry registry = mlirDialectRegistryCreate();
  mlirRegisterAllDialects(registry);
  MlirContext context =
      mlirContextCreateWithRegistry(registry, /*threadingEnabled=*/false);
  mlirContextSetThreadPool(context, threadPool);
  mlirContextDestroy(context);
  mlirDialectRegistryDestroy(registry);
  mlirLlvmThreadPoolDestroy(threadPool);
}

void testDiagnostics(void) {
  MlirContext ctx = mlirContextCreate();
  MlirDiagnosticHandlerID id = mlirContextAttachDiagnosticHandler(
      ctx, errorHandler, (void *)42, deleteUserData);
  fprintf(stderr, "@test_diagnostics\n");
  MlirLocation unknownLoc = mlirLocationUnknownGet(ctx);
  mlirEmitError(unknownLoc, "test diagnostics");
  MlirAttribute unknownAttr = mlirLocationGetAttribute(unknownLoc);
  MlirLocation unknownClone = mlirLocationFromAttribute(unknownAttr);
  mlirEmitError(unknownClone, "test clone");
  MlirLocation fileLineColLoc = mlirLocationFileLineColGet(
      ctx, mlirStringRefCreateFromCString("file.c"), 1, 2);
  mlirEmitError(fileLineColLoc, "test diagnostics");
  MlirLocation fileLineColRange = mlirLocationFileLineColRangeGet(
      ctx, mlirStringRefCreateFromCString("other-file.c"), 1, 2, 3, 4);
  mlirEmitError(fileLineColRange, "test diagnostics");
  MlirLocation callSiteLoc = mlirLocationCallSiteGet(
      mlirLocationFileLineColGet(
          ctx, mlirStringRefCreateFromCString("other-file.c"), 2, 3),
      fileLineColLoc);
  mlirEmitError(callSiteLoc, "test diagnostics");
  MlirLocation null = {0};
  MlirLocation nameLoc =
      mlirLocationNameGet(ctx, mlirStringRefCreateFromCString("named"), null);
  mlirEmitError(nameLoc, "test diagnostics");
  MlirLocation locs[2] = {nameLoc, callSiteLoc};
  MlirAttribute nullAttr = {0};
  MlirLocation fusedLoc = mlirLocationFusedGet(ctx, 2, locs, nullAttr);
  mlirEmitError(fusedLoc, "test diagnostics");
  mlirContextDetachDiagnosticHandler(ctx, id);
  mlirEmitError(unknownLoc, "more test diagnostics");
  // CHECK-LABEL: @test_diagnostics
  // CHECK: processing diagnostic (userData: 42) <<
  // CHECK:   test diagnostics
  // CHECK:   loc(unknown)
  // CHECK: processing diagnostic (userData: 42) <<
  // CHECK:   test clone
  // CHECK:   loc(unknown)
  // CHECK: >> end of diagnostic (userData: 42)
  // CHECK: processing diagnostic (userData: 42) <<
  // CHECK:   test diagnostics
  // CHECK:   loc("file.c":1:2)
  // CHECK: >> end of diagnostic (userData: 42)
  // CHECK: processing diagnostic (userData: 42) <<
  // CHECK:   test diagnostics
  // CHECK:   loc("other-file.c":1:2 to 3:4)
  // CHECK: >> end of diagnostic (userData: 42)
  // CHECK: processing diagnostic (userData: 42) <<
  // CHECK:   test diagnostics
  // CHECK:   loc(callsite("other-file.c":2:3 at "file.c":1:2))
  // CHECK: >> end of diagnostic (userData: 42)
  // CHECK: processing diagnostic (userData: 42) <<
  // CHECK:   test diagnostics
  // CHECK:   loc("named")
  // CHECK: >> end of diagnostic (userData: 42)
  // CHECK: processing diagnostic (userData: 42) <<
  // CHECK:   test diagnostics
  // CHECK:   loc(fused["named", callsite("other-file.c":2:3 at "file.c":1:2)])
  // CHECK: deleting user data (userData: 42)
  // CHECK-NOT: processing diagnostic
  // CHECK:     more test diagnostics
  mlirContextDestroy(ctx);
}

int testBlockPredecessorsSuccessors(MlirContext ctx) {
  // CHECK-LABEL: @testBlockPredecessorsSuccessors
  fprintf(stderr, "@testBlockPredecessorsSuccessors\n");

  const char *moduleString = "module {\n"
                             "  func.func @test(%arg0: i32, %arg1: i16) {\n"
                             "    cf.br ^bb1(%arg1 : i16)\n"
                             "  ^bb1(%0: i16):  // pred: ^bb0\n"
                             "    cf.br ^bb2(%arg0 : i32)\n"
                             "  ^bb2(%1: i32):  // pred: ^bb1\n"
                             "    return\n"
                             "  }\n"
                             "}\n";

  MlirModule module =
      mlirModuleCreateParse(ctx, mlirStringRefCreateFromCString(moduleString));

  MlirOperation moduleOp = mlirModuleGetOperation(module);
  MlirRegion moduleRegion = mlirOperationGetRegion(moduleOp, 0);
  MlirBlock moduleBlock = mlirRegionGetFirstBlock(moduleRegion);
  MlirOperation function = mlirBlockGetFirstOperation(moduleBlock);
  MlirRegion funcRegion = mlirOperationGetRegion(function, 0);
  MlirBlock entryBlock = mlirRegionGetFirstBlock(funcRegion);
  MlirBlock middleBlock = mlirBlockGetNextInRegion(entryBlock);
  MlirBlock successorBlock = mlirBlockGetNextInRegion(middleBlock);

#define FPRINTF_OP(OP, FMT) fprintf(stderr, #OP ": " FMT "\n", OP)

  // CHECK: mlirBlockGetNumPredecessors(entryBlock): 0
  FPRINTF_OP(mlirBlockGetNumPredecessors(entryBlock), "%ld");

  // CHECK: mlirBlockGetNumSuccessors(entryBlock): 1
  FPRINTF_OP(mlirBlockGetNumSuccessors(entryBlock), "%ld");
  // CHECK: mlirBlockEqual(middleBlock, mlirBlockGetSuccessor(entryBlock, 0)): 1
  FPRINTF_OP(mlirBlockEqual(middleBlock, mlirBlockGetSuccessor(entryBlock, 0)),
             "%d");
  // CHECK: mlirBlockGetNumPredecessors(middleBlock): 1
  FPRINTF_OP(mlirBlockGetNumPredecessors(middleBlock), "%ld");
  // CHECK: mlirBlockEqual(entryBlock, mlirBlockGetPredecessor(middleBlock, 0))
  FPRINTF_OP(
      mlirBlockEqual(entryBlock, mlirBlockGetPredecessor(middleBlock, 0)),
      "%d");

  // CHECK: mlirBlockGetNumSuccessors(middleBlock): 1
  FPRINTF_OP(mlirBlockGetNumSuccessors(middleBlock), "%ld");
  // CHECK: BlockEqual(successorBlock, mlirBlockGetSuccessor(middleBlock, 0)): 1
  fprintf(
      stderr,
      "BlockEqual(successorBlock, mlirBlockGetSuccessor(middleBlock, 0)): %d\n",
      mlirBlockEqual(successorBlock, mlirBlockGetSuccessor(middleBlock, 0)));
  // CHECK: mlirBlockGetNumPredecessors(successorBlock): 1
  FPRINTF_OP(mlirBlockGetNumPredecessors(successorBlock), "%ld");
  // CHECK: Equal(middleBlock, mlirBlockGetPredecessor(successorBlock, 0)): 1
  fprintf(
      stderr,
      "Equal(middleBlock, mlirBlockGetPredecessor(successorBlock, 0)): %d\n",
      mlirBlockEqual(middleBlock, mlirBlockGetPredecessor(successorBlock, 0)));

  // CHECK: mlirBlockGetNumSuccessors(successorBlock): 0
  FPRINTF_OP(mlirBlockGetNumSuccessors(successorBlock), "%ld");

#undef FPRINTF_OP

  mlirModuleDestroy(module);

  return 0;
}

int main(void) {
  MlirContext ctx = mlirContextCreate();
  registerAllUpstreamDialects(ctx);
  mlirContextGetOrLoadDialect(ctx, mlirStringRefCreateFromCString("func"));
  mlirContextGetOrLoadDialect(ctx, mlirStringRefCreateFromCString("memref"));
  mlirContextGetOrLoadDialect(ctx, mlirStringRefCreateFromCString("shape"));
  mlirContextGetOrLoadDialect(ctx, mlirStringRefCreateFromCString("scf"));

  if (constructAndTraverseIr(ctx))
    return 1;
  buildWithInsertionsAndPrint(ctx);
  if (createOperationWithTypeInference(ctx))
    return 2;

  if (printBuiltinTypes(ctx))
    return 3;
  if (printBuiltinAttributes(ctx))
    return 4;
  if (printAffineMap(ctx))
    return 5;
  if (printAffineExpr(ctx))
    return 6;
  if (affineMapFromExprs(ctx))
    return 7;
  if (printIntegerSet(ctx))
    return 8;
  if (registerOnlyStd())
    return 9;
  if (testBackreferences())
    return 10;
  if (testOperands())
    return 11;
  if (testClone())
    return 12;
  if (testTypeID(ctx))
    return 13;
  if (testSymbolTable(ctx))
    return 14;
  if (testDialectRegistry())
    return 15;
  if (testOperationWalk(ctx))
    return 16;

  testExplicitThreadPools();
  testDiagnostics();

  if (testBlockPredecessorsSuccessors(ctx))
    return 17;

  // CHECK: DESTROY MAIN CONTEXT
  // CHECK: reportResourceDelete: resource_i64_blob
  fprintf(stderr, "DESTROY MAIN CONTEXT\n");
  mlirContextDestroy(ctx);

  return 0;
}
